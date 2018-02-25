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
#include "adc.h"
#include "diagnostics.h"
#include "file_paths.h"
#include "gps_core.h"
#include "led.h"
#include "logger_core.h"
#include "logger_frames.h"
#include <FreeRTOS/include/queue.h>
#include <misc.h>
#include "power.h"
#include <stdbool.h>
#include <string.h>

#define DEBUG_ID DEBUG_ID_LOGGER_CORE
#include <debug.h>

#define QUEUE_LENGTH_PIDS 50
#define WRITE_CHUNK_SIZE 512

/* --------- public data ---------------- */

/* --------- private data --------------- */
static QueueHandle_t _pid_queue_handle;
static volatile bool _log_gps_request = true; //this can be modified from another task
static volatile bool _log_acceleration_request; //this can be modified from another task
static volatile bool _log_diagostics_request = true; //this can be modified from another task
static volatile bool _log_battery_voltage_request; //this can be modified from another task
static uint8_t _write_chunk_buffer[WRITE_CHUNK_SIZE];
static uint32_t _write_chunk_index;
static FIL *_log_file_handle_ptr;

/* --------- private prototypes --------- */
static void log_frame(uint8_t frame_length, const uint8_t *frame_ptr);
static void log_gps_INTERNAL(void);
static void log_diagnostics_INTERNAL(void);
static void log_battery_voltage_INTERNAL(void);

/* ----------- implementation ----------- */

//log_pid can be called from another task
void log_pid(pid_mode_t mode, uint8_t pid, uint8_t a, uint8_t b, uint8_t c, uint8_t d){
	frame_pid_t frame;
	memset(&frame, 0, sizeof(frame));
	frame.frame_type = logger_frame_pid;
	frame.mode = mode;
	frame.pid = pid;
	frame.a = a;
	frame.b = b;
	frame.c = c;
	frame.d = d;
	frame.timestamp = xTaskGetTickCount();
	if (unlikely(xQueueSendToBack(_pid_queue_handle, &frame, 0/*don't wait if queue is full*/)) == errQUEUE_FULL){
		GLOBAL_diagnostics_frame.pid_queue_blocks++;
	}
}

void log_detected_protocol(obd_protocol_t protocol){
	static bool logged_once = false;
	if (logged_once == false){
		debugf("Saving detected protocol %d", protocol);
		//this will be called from another task so it must be synchronized with a queue
		frame_pid_t frame;
		frame.frame_type = logger_frame_save_used_protocol;
		frame.a = protocol;
		if (unlikely(xQueueSendToBack(_pid_queue_handle, &frame, 0/*don't wait if queue is full*/)) == errQUEUE_FULL){
			GLOBAL_diagnostics_frame.pid_queue_blocks++;
		}
		logged_once = true;
	}
}

void log_gps(void){
	_log_gps_request = true;
}

void log_acceleration(void){
	_log_acceleration_request = true;
}

void log_internal_diagnostics(void){
	_log_diagostics_request = true;
}

void log_battery_voltage(void){
	_log_battery_voltage_request = true;
}

void log_init(FIL *file_handle_ptr){
	static StaticQueue_t queue_control_struct;
	static uint8_t pid_queue[QUEUE_LENGTH_PIDS*sizeof(frame_pid_t)];

	_log_file_handle_ptr = file_handle_ptr;

	_pid_queue_handle = xQueueCreateStatic(
			QUEUE_LENGTH_PIDS,
			sizeof(frame_pid_t),
			pid_queue,
			&queue_control_struct);
}

uint32_t log_task(void){
	frame_pid_t frame;
	uint32_t saved_frames_counter = 0;
	while (xQueueReceive(_pid_queue_handle, &frame, 0/*don't wait*/) == pdTRUE){

		if (unlikely(frame.frame_type == logger_frame_save_used_protocol)){

			/*_file_handle is static to reduce stack usage.
			 * TODO: To reduce ram usage - close main log file (_log_file_handle_ptr),
			 * reuse the FIL object to save the used protocol, reopen the log file in append mode.
			 */
			static FIL _file_handle;
			FRESULT r = f_open(&_file_handle, PROTOCOL_FILE_PATH, FA_CREATE_ALWAYS | FA_WRITE);
			debugf("protocol file open file status = %d", r);
			if (r == FR_OK || r == FR_EXIST){
				char buff[5];
				sprintf(buff, "%d", frame.a);
				uint32_t bytes_written = 0;
				f_write(&_file_handle, buff, strlen(buff), &bytes_written);
				debugf("Written %ld bytes <%s>", bytes_written, buff);
				f_close(&_file_handle);
			} else {
				debugf("Could not open detected protocol file");
			}

		} else { //normal frame to log
			//			debugf("Got frame to log");
			log_frame(sizeof(frame), (const uint8_t*)&frame);
			saved_frames_counter++;
		}
	}

	if (unlikely(_log_gps_request)){
		_log_gps_request = false;
		log_gps_INTERNAL();
	}

	if (unlikely(_log_diagostics_request)){
		_log_diagostics_request = false;
		log_diagnostics_INTERNAL();
	}

	if (unlikely(_log_battery_voltage_request)){
		_log_battery_voltage_request = false;
		log_battery_voltage_INTERNAL();
	}
	return saved_frames_counter;
}

static void log_frame(uint8_t frame_length, const uint8_t *frame_ptr){
	//buffer data to be written to minimize SPI IO operations
	if (_write_chunk_index + frame_length + 3/*start,length,checksum bytes*/ >= sizeof(_write_chunk_buffer)){
		log_flush(); //this function rewinds _write_chunk_index
	}

	_write_chunk_buffer[_write_chunk_index] = 0xCA; //start of frame flag
	_write_chunk_index++;
	_write_chunk_buffer[_write_chunk_index] = frame_length;
	_write_chunk_index++;

	memcpy(_write_chunk_buffer + _write_chunk_index, frame_ptr, frame_length);
	_write_chunk_index += frame_length;

	uint8_t checksum = 0;
	for (uint32_t i = 0; i < frame_length; i++){
		checksum += *frame_ptr;
		frame_ptr++;
	}

	_write_chunk_buffer[_write_chunk_index] = checksum;
	_write_chunk_index++;
}

void log_flush(void){
	uint32_t bytes_written = 0;
	f_write(_log_file_handle_ptr, _write_chunk_buffer, _write_chunk_index, &bytes_written);
	debugf("Written %ld bytes, buffer had %ld", bytes_written, _write_chunk_index);
	_write_chunk_index = 0;
	if (GLOBAL_power_failure_flag == false){
		led_blink_request(LED_SD_CARD);
	}
}

static void log_gps_INTERNAL(void){
	if (GLOBAL_frame_gps_current.valid == false){
		return; //don't log invalid frames
	}
	log_frame(sizeof(GLOBAL_frame_gps_current), (const uint8_t*)&GLOBAL_frame_gps_current);
}

static void log_diagnostics_INTERNAL(void){
	GLOBAL_diagnostics_frame.timestamp = xTaskGetTickCount();
	log_frame(sizeof(GLOBAL_diagnostics_frame), (const uint8_t*)&GLOBAL_diagnostics_frame);
}

static void log_battery_voltage_INTERNAL(void){
	frame_battery_voltage_t frame;
	memset(&frame, 0, sizeof(frame));
	frame.frame_type = logger_frame_battery_voltage;
	frame.timestamp = xTaskGetTickCount();
	frame.power_failure_flag = GLOBAL_power_failure_flag;
	frame.battery_voltage_adc_code = adc_get_result();

	log_frame(sizeof(frame), (const uint8_t*)&frame);
}
