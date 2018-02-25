/*
Open OBD2 datalogger
Copyright (C) 2018 Artur Langner

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "acquisition_task.h"
#include "adc.h"
#include "application_tasks.h"
#include <crash_handler.h>
#include <FatFS/ff.h>
#include "file_paths.h"
#include <gps_core.h>
#include <gps_uart.h>
#include "logger_core.h"
#include <misc.h>
#include <pins.h>
#include "power.h"
#include "rtt_console.h"
#include <stdlib.h>
#include <string.h>
#include "storage_task.h"
#include "timer.h"

#define DEBUG_ID DEBUG_ID_STORAGE_TASK
#include <debug.h>

static FATFS _fat;
static FIL _file_handle;

static void log_filename_migration_subtask(void);
__attribute__((noreturn)) static void blink_of_death(void);
static void load_config_file(FIL *config_file_handle);
static void config_parse_line(uint32_t argc, char *argv[]);

void storage_task(void *params __attribute__((unused))){

	power_init();

	adc_init();

	gps_uart_init();

	FRESULT r;
	for (uint32_t i = 0; i < 5; i++){
		r = f_mount(&_fat, "", 1/*force mount now*/);
		debugf("attempt %ld mount = %d", i, r);
		if (r == FR_OK){
			break;
		} else {
			SD_POWER_OFF();
			vTaskDelay(pdMS_TO_TICKS(2000)/*ms*/);
		}
	}

	if (r){ //failed to mount SD card - blink of death
		blink_of_death();
	}

	r = f_mkdir("obdlog");
	debugf("mkdir = %d", r);
	if (r != FR_OK && r != FR_EXIST){
		blink_of_death();
	}

	debug_file_init();

	gps_uart_request_wake(); //SD card initialization should take some time.
	//This will give time the GPS to start after power-on.

	//read configuration
	bool use_default_config = true;
	r = f_open(&_file_handle, CONFIG_PATH, FA_READ);
	if (r == FR_OK){
		use_default_config = false;
		load_config_file(&_file_handle);
		f_close(&_file_handle);
	}

	//read previously used protocol
	obd_protocol_t first_protocol_to_try = obd_proto_auto;
	r = f_open(&_file_handle, PROTOCOL_FILE_PATH, FA_READ);
	debugf("last protocol file open file status = %d", r);
	if (r == FR_OK){
		char buffer[5];
		f_gets(buffer, sizeof(buffer), &_file_handle);
		buffer[4] = '\0';
		first_protocol_to_try = atoi(buffer);
		debugf("First protocol to try is %d <%s>", first_protocol_to_try, buffer);
		f_close(&_file_handle);
	}

	//open the logfile
	r = f_open(&_file_handle, TMP_LOG_PATH, FA_CREATE_ALWAYS | FA_WRITE);
	debugf("log file open file status = %d", r);
	if (r != FR_OK && r != FR_EXIST){
		blink_of_death();
	}

	LED1_ON();

	gps_uart_init();
	log_init(&_file_handle);

	debugf("SRSID %08X <%s>", (unsigned int)SIM_SRSID, crash_handler_get_info());

	acquisition_start(first_protocol_to_try, use_default_config); //unblock acquisition task

	while (1){
		if (GLOBAL_power_failure_flag){
			debugf("Power drop"); //this may be a temporary glitch (eg. wipers or fans being turned on)
			storage_sync();
			debug_file_task();
			debug_sync();

			vTaskDelay(pdMS_TO_TICKS(20)); //power may die at this point
			vTaskResume(&acquisition_task_handle); //continue running
			GLOBAL_power_failure_flag = false;
		} else {
			debug_file_task();
			gps_uart_task();
			static uint32_t frames_saved = 0;
			frames_saved += log_task();
			log_filename_migration_subtask();
			rtt_console_task();
			static TickType_t last_check = 0;

			//housekeeping every 20s - flush buffers, check stack
			if (xTaskGetTickCount() - last_check > pdMS_TO_TICKS(20000)){
				last_check = xTaskGetTickCount();
				uint32_t stack_water_mark = uxTaskGetStackHighWaterMark(&storage_task_handle);
				debugf("stack left %ld, frames %ld, GPS %d %02d%02d%02d %02d%02d%02d",
						stack_water_mark*sizeof(UBaseType_t),
						frames_saved,
						GLOBAL_frame_gps_current.valid,
						GLOBAL_frame_gps_current.date.year,
						GLOBAL_frame_gps_current.date.month,
						GLOBAL_frame_gps_current.date.day,
						GLOBAL_frame_gps_current.time.hours,
						GLOBAL_frame_gps_current.time.minutes,
						GLOBAL_frame_gps_current.time.seconds);
				frames_saved = 0;

				storage_sync();
				debug_sync();
			}

			if (GLOBAL_power_failure_flag == false){ //else don't sleep - loop and flush the buffers immediately
				if (power_is_good() == false){
					debugf("Voltage is too low - shutting down");
					storage_sync();
					debug_file_task();
					debug_sync();
					power_shutdown();
				}
				vTaskDelay(2);
			}

			if (GLOBAL_frame_gps_current.valid){
				LED3_ON();
			} else {
				LED3_OFF();
			}
		}
	} //end of task loop
}

static void log_filename_migration_subtask(void){
	/* At startup the obdlog/noname.log file is created/overwritten. Logging starts
	 * immediately. When GPS has acquired a fix with valid time and date the temporary
	 * name is moved to obdlog/YEAR/MONTH/DAYHOURMINUTESECOND.log
	 *
	 * Example: "obdlog/2017/06/08101509.log" //2017, 8th June, 10:15 UTC
	 */
	static bool migrated = false;
	if (unlikely(migrated == false)){
		if (GLOBAL_frame_gps_current.valid && GLOBAL_frame_gps_current.time.seconds){

			//flush the debug buffer, so messages from this block will not get dropped
			debug_file_task();
			debug_sync();

			char scratchpad[32];
			FRESULT r;

			//create year directory
			snprintf(scratchpad, sizeof(scratchpad), "obdlog/%02d", GLOBAL_frame_gps_current.date.year);
			r = f_mkdir(scratchpad);
			debugf("mkdir %s %d", scratchpad, r);
			if (r != FR_OK && r != FR_EXIST){
				blink_of_death();
			}

			//create month directory
			snprintf(scratchpad, sizeof(scratchpad), "obdlog/%02d/%02d",
					GLOBAL_frame_gps_current.date.year,
					GLOBAL_frame_gps_current.date.month);
			r = f_mkdir(scratchpad);
			debugf("mkdir %s %d", scratchpad, r);
			if (r != FR_OK && r != FR_EXIST){
				blink_of_death();
			}

			//close temporary file (FAT may be otherwise damaged)
			r = f_close(&_file_handle);
			debugf("close %d", r);
			if (r != FR_OK && r != FR_EXIST){
				blink_of_death();
			}

			//rename temporary file
			snprintf(scratchpad, sizeof(scratchpad), "obdlog/%02d/%02d/%02d%02d%02d%02d.log",
					GLOBAL_frame_gps_current.date.year,
					GLOBAL_frame_gps_current.date.month,
					GLOBAL_frame_gps_current.date.day,
					GLOBAL_frame_gps_current.time.hours,
					GLOBAL_frame_gps_current.time.minutes,
					GLOBAL_frame_gps_current.time.seconds);
			r = f_rename(TMP_LOG_PATH, scratchpad);
			debugf("rename to %s %d", scratchpad, r);
			if (r != FR_OK && r != FR_EXIST){
				blink_of_death();
			}

			//open final file, move pointer to the end
			r = f_open(&_file_handle, scratchpad, FA_OPEN_APPEND | FA_WRITE);
			debugf("open file status = %d", r);
			if (r != FR_OK && r != FR_EXIST){
				blink_of_death();
			}

			migrated = true;
		}
	}
}

#define MAX_OPTIONS 5
static void load_config_file(FIL *config_file_handle){
	char line_buffer[64];
	while (f_gets(line_buffer, sizeof(line_buffer), config_file_handle)){
		//		debugf("Line <%s>", line_buffer);
		if (line_buffer[0] == '#'){ //ignore comment lines
			debugf("Comment line");
			continue;
		}
		//tokenize the command string by spaces
		char* argv[MAX_OPTIONS];
		uint32_t argc = 1;
		uint32_t original_length = strlen(line_buffer);
		argv[0] = line_buffer;
		for (uint8_t i = 0; i < original_length; i++){
			if (line_buffer[i] == ' '){ //space separates command arguments
				line_buffer[i] = '\0';
				argv[argc] = line_buffer + i + 1;
				argc++;
				if (argc > MAX_OPTIONS-1){
					debugf("too many arguments");
					continue;
				}
			}
		}
		config_parse_line(argc, argv);
	}
}

static void config_parse_line(uint32_t argc, char *argv[]){
	//each config line has the following format:
	//TYPE PID_MODE PID SAMPLING_INTERVAL

	if (argc != 4){
		debugf("Wrong number of options in line? %ld", argc);
	}

	acquisition_channel_t channel;

	channel.channel_type = atoi(argv[0]);
	if (channel.channel_type != logger_frame_pid &&
			channel.channel_type != logger_frame_gps &&
			channel.channel_type != logger_frame_acceleration &&
			channel.channel_type != logger_frame_internal_diagnostics){
		debugf("Wrong channel type! %d", channel.channel_type);
		return;
	}

	if (channel.channel_type == logger_frame_pid){
		channel.pid_mode = atoi(argv[1]);
		if (channel.pid_mode != pid_mode_01 &&
				channel.pid_mode != pid_mode_02 &&
				channel.pid_mode != pid_mode_03 &&
				channel.pid_mode != pid_mode_04 &&
				channel.pid_mode != pid_mode_05&&
				channel.pid_mode != pid_mode_09){
			debugf("Wrong PID mode! %d", channel.pid_mode);
		}
	}

	channel.pid = atoi(argv[2]);

	uint32_t sampling_interval_seconds = atoi(argv[3]);
	if (sampling_interval_seconds < 1){
		debugf("Interval is smaller that 1 second (is %ld)", sampling_interval_seconds);
	}

	channel.interval = pdMS_TO_TICKS(sampling_interval_seconds * 1000);
	channel.next_sample_timestamp = 0;

	acquisition_add_channel(&channel);
}

void storage_sync(void){
	log_flush();
	f_sync(&_file_handle);
}

__attribute__((noreturn)) static void blink_of_death(void){
	debugf("panic");
	while(1){
		rtt_console_task();
		LED1_ON();
		LED2_ON();
		LED3_ON();
		vTaskDelay(pdMS_TO_TICKS(500/*ms*/));
		LED1_OFF();
		LED2_OFF();
		LED3_OFF();
		vTaskDelay(pdMS_TO_TICKS(500/*ms*/));

		if (!power_is_good()){
			//prevent draining the battery in case the device wakes up but
			//something is for example wrong with the SD card
			power_shutdown(); //never returns - performs a reset at wakeup
		}
	}
}
