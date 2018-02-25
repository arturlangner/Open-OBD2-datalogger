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
#define DEBUG_ID DEBUG_ID_DEBUG //must be defined before including debug.h

#include "debug.h"
#include <FatFS/ff.h>
#include "file_paths.h"
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/semphr.h>
#include <FreeRTOS/include/task.h>
#include <MKE06Z4.h>
#include <inttypes.h>
#include <proginfo.h>
#include <SEGGER/SEGGER_RTT.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "version.h"

#define MAX_LINE_LENGTH 100
#define DEBUG_QUEUE_LENGTH 15

static StaticSemaphore_t _RTT_mutex;
static SemaphoreHandle_t _RTT_mutex_handle;
static QueueHandle_t _debug_log_queue_handle;

static FIL _debug_log_file_handle;
static bool _debug_file_ready = false;

static bool _debug_channel_enable[DEBUG_ID_COUNT] = {
		[DEBUG_ID_DISABLED]         = false,
		[DEBUG_ID_MAIN]             = false,
		[DEBUG_ID_SPI]              = false,
		[DEBUG_ID_OBD]              = false,
		[DEBUG_ID_OBD_CAN]          = false,
		[DEBUG_ID_OBD_K_LINE]       = false,
		[DEBUG_ID_OBD_UART]         = false,
		[DEBUG_ID_ACQUISITION_TASK] = false,
		[DEBUG_ID_STORAGE_TASK]     = false,
		[DEBUG_ID_LOGGER_CORE]      = false,
		[DEBUG_ID_GPS_UART]         = false,
		[DEBUG_ID_GPS_CORE]         = false,
		[DEBUG_ID_CONSOLE]          = true,
		[DEBUG_ID_DEBUG]            = true,
};

static const bool DEBUG_CHANNELS_ENABLED_UNDER_DEBUGGER[DEBUG_ID_COUNT] = {
		[DEBUG_ID_DISABLED]         = false,
		[DEBUG_ID_MAIN]             = false,
		[DEBUG_ID_SPI]              = false,
		[DEBUG_ID_OBD]              = true,
		[DEBUG_ID_OBD_CAN]          = false,
		[DEBUG_ID_OBD_K_LINE]       = false,
		[DEBUG_ID_OBD_UART]         = false,
		[DEBUG_ID_ACQUISITION_TASK] = true,
		[DEBUG_ID_STORAGE_TASK]     = true,
		[DEBUG_ID_LOGGER_CORE]      = true,
		[DEBUG_ID_GPS_UART]         = false,
		[DEBUG_ID_GPS_CORE]         = false,
		[DEBUG_ID_CONSOLE]          = true,
		[DEBUG_ID_DEBUG]            = true,
};

void debug_printf(debug_id_t id, const char *format, ...){
	if (_debug_channel_enable[id]){
		va_list args;
		va_start(args, format);

		char line_buffer[MAX_LINE_LENGTH];
		int prefix_length = snprintf(line_buffer, sizeof(line_buffer), "%ld", (uint32_t)xTaskGetTickCount());
		int line_length = vsnprintf(line_buffer+prefix_length, sizeof(line_buffer)-prefix_length-1, format, args);

		line_buffer[sizeof(line_buffer)-1] = '\0';

		xSemaphoreTake(_RTT_mutex_handle, portMAX_DELAY);
		SEGGER_RTT_Write(0/*BufferIndex*/, line_buffer, prefix_length+line_length);
		xSemaphoreGive(_RTT_mutex_handle);

		if (_debug_file_ready){
			if (xQueueSendToBack(_debug_log_queue_handle, line_buffer, 0/*don't wait if queue is full*/) == errQUEUE_FULL){
				//TODO: count lost lines
			}
		}
	}
}

void debug_init(void){
	_RTT_mutex_handle = xSemaphoreCreateMutexStatic(&_RTT_mutex);
//	memcpy(_debug_channel_enable, DEBUG_CHANNELS_ENABLED_UNDER_DEBUGGER, sizeof(_debug_channel_enable));

	static StaticQueue_t queue_control_struct;
	static uint8_t debug_log_queue[MAX_LINE_LENGTH * DEBUG_QUEUE_LENGTH];

	_debug_log_queue_handle = xQueueCreateStatic(
			DEBUG_QUEUE_LENGTH,
			MAX_LINE_LENGTH,
			debug_log_queue,
			&queue_control_struct);
}

void debug_enable_id(debug_id_t id, bool enable){
	if (id < DEBUG_ID_COUNT){
		debugf("Debug channel %d enable %d", id, enable);
		_debug_channel_enable[id] = enable;
	} else {
		debugf("Wrong channel ID");
	}
}

void debug_file_init(void){
	FRESULT r;

	//read debug config file
	r = f_open(&_debug_log_file_handle, DEBUG_CONFIG_FILE_PATH, FA_READ);
	debugf("debug config open file status = %d", r);
	bool debug_file_enabled = false;
	if (r == FR_OK){
		debug_file_enabled = true;
		char buffer[DEBUG_ID_COUNT*3/*assume 2 digit numbers + space*/+3];
		f_gets(buffer, sizeof(buffer), &_debug_log_file_handle);
		buffer[sizeof(buffer)-1] = '\0';

		//the text file has the following format "1 5 10 12 15 16 17" - numbers separated by spaces
		char *p = buffer;
		char *number_start = buffer;

		while (*p){
			if (*p == ' '){
				*p = '\0';
				int x = atoi(number_start);
				debug_enable_id(x, true);
				p++;
				number_start = p;
			}
			p++;
		}
		f_close(&_debug_log_file_handle);
	}

	if (debug_file_enabled == false){
		return;
	}

	r = f_mkdir("obdlog/debug");
	debugf("mkdir = %d", r);
	if (r != FR_OK && r != FR_EXIST){
		debugf("Can't create debug directory!");
		return;
	}

	//find the next debug log filename
	uint32_t log_file_number = 0;
	DIR dir;
	FILINFO fno;
	r = f_opendir(&dir, DEBUG_FILE_DIRECTORY);                       /* Open the directory */
	if (r == FR_OK) {
		for (;;) {
			r = f_readdir(&dir, &fno); //read directory item
			if (r != FR_OK || fno.fname[0] == 0){
				break; //all elements in directory have been read
			}
			if ((fno.fattrib & AM_DIR) == 0) { //it is a file, not directory
				if (fno.fname[0] == 'L' && //look only for files like "log12345.txt"
						fno.fname[1] == 'O' &&
						fno.fname[2] == 'G' &&
						fno.fname[9] == 'T' &&
						fno.fname[10] == 'X' &&
						fno.fname[11] == 'T'
				){
					fno.fname[8] = '\0';
					uint32_t v = atoi(fno.fname+3);
					if (v >= log_file_number){
						log_file_number = v + 1;
					}
				}
			}
		}
		f_closedir(&dir);
	} else {
		debugf("No debug directory?");
	}

	//create debug log file
	char debug_file_path[27];
	snprintf(debug_file_path, sizeof(debug_file_path), DEBUG_FILE_PATH_FORMAT, (unsigned int)log_file_number);
	debugf("Saving debug to file %s", debug_file_path);
	r = f_open(&_debug_log_file_handle, debug_file_path, FA_CREATE_ALWAYS | FA_WRITE);
	debugf("debug file open file status = %d", r);
	if (r == FR_OK || r == FR_EXIST){
		_debug_file_ready = true;

		prog_info_t *pi = (prog_info_t*)PROG_INFO_OFFSET;

		debugf("Debug started, bootloader %d.%d, application %d.%d",
				pi->version_major, pi->version_minor,
				SOFTWARE_VERSION_MAJOR, SOFTWARE_VERSION_MINOR);
	}
}

void debug_file_task(void){
	if (_debug_file_ready){
		char line_buffer[MAX_LINE_LENGTH];
		while (xQueueReceive(_debug_log_queue_handle, line_buffer, 0/*don't wait*/) == pdTRUE){
			uint32_t bytes_written;
			f_write(&_debug_log_file_handle, line_buffer, strlen(line_buffer), &bytes_written);
		}
	}
}

void debug_sync(void){
	if (_debug_file_ready){
		f_sync(&_debug_log_file_handle);
	}
}
