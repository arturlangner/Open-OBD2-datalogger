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
#include "application_tasks.h"
#include "diagnostics.h"
#include "logger_core.h"
#include <obd/obd.h>
#include <obd/obd_pids.h>
#include <stdbool.h>
#include <string.h>

#define DEBUG_ID DEBUG_ID_ACQUISITION_TASK
#include <debug.h>

#define MAX_CHANNELS 150
#define MAX_PID_FAILURES 3

#define AUTODETECT_PIDS_MASK 0x80 //this can't overlap obd_protocol_t bits

static acquisition_channel_t _channel[MAX_CHANNELS];
static uint32_t _channel_count;

static volatile obd_protocol_t _startup_protocol = obd_proto_none;

static void autodetect_pids(void);

void acquisition_task(void *params __attribute__((unused))){

	//block until channels are added by storage task
	while (_startup_protocol == obd_proto_none){ //this will be changed by the other task
		vTaskDelay(5);
	}

	obd_init(_startup_protocol & ~AUTODETECT_PIDS_MASK/*remove the special bit*/);

	if (_startup_protocol & AUTODETECT_PIDS_MASK){
		autodetect_pids();
	}

	debugf("Starting acquisition loop, %ld channels total", _channel_count);

	while (1){
		uint32_t sleep_time_ticks = pdMS_TO_TICKS(1000); //default sleep time

		obd_task();

		for (uint32_t i = 0; i < _channel_count; i++){ //loop through all channels
			if (xTaskGetTickCount() >= _channel[i].next_sample_timestamp){
				switch (_channel[i].channel_type){
				case logger_frame_pid:
					do {
						obd_pid_response_t pid_response;
						int32_t status = obd_get_pid(_channel[i].pid_mode, _channel[i].pid, &pid_response);
						if (status > 0){
							log_pid(_channel[i].pid_mode,
									_channel[i].pid,
									pid_response.byte_a,
									pid_response.byte_b,
									pid_response.byte_c,
									pid_response.byte_d);
							_channel[i].failure_count = 0;

//							debugf("Read channel %d PID %02X", (unsigned int)i,	_channel[i].pid);
						} else {
							_channel[i].failure_count++;
							if (_channel[i].failure_count > MAX_PID_FAILURES){
								_channel[i].channel_type = logger_frame_disabled;
								debugf("Disabling channel %d PID %02X - too many failures",
										(unsigned int)i,
										_channel[i].pid);
							}
						}
						vTaskDelay(pdMS_TO_TICKS(40));
					} while (0);
					break;
				case logger_frame_gps: log_gps(); break;
				case logger_frame_acceleration: log_acceleration(); break;
				case logger_frame_internal_diagnostics: log_internal_diagnostics(); break;
				case logger_frame_battery_voltage: log_battery_voltage(); break;
				default: break; //this also handles logger_frame_disabled
				}

				if (_channel[i].channel_type != logger_frame_disabled){
					if (_channel[i].interval){ //normal sampling interval
						_channel[i].next_sample_timestamp = xTaskGetTickCount() + _channel[i].interval;
						debugf("Channel %ld next sample time %ld", i, _channel[i].next_sample_timestamp);
						if (_channel[i].interval < sleep_time_ticks){
							sleep_time_ticks = _channel[i].interval;
						}
					} else { //if sampling interval is zero - sample only once and disable further sampling
						_channel[i].channel_type = logger_frame_disabled;
						debugf("Channel %ld sampled once, disabling", i);
					}
				}
			}
		}

		static TickType_t last_check = 0;
		if (xTaskGetTickCount() - last_check > pdMS_TO_TICKS(20000)){
			UBaseType_t stack_water_mark = uxTaskGetStackHighWaterMark(&acquisition_task_handle);
			last_check = xTaskGetTickCount();
			debugf("time %ld stack left %ld next sleep %ld ticks", last_check, stack_water_mark*sizeof(UBaseType_t), sleep_time_ticks);
		}

		vTaskDelay(sleep_time_ticks);
	} //end of task loop
} //end of acquisition_task

void acquisition_start(obd_protocol_t first_protocol_to_try, bool use_default_config){
	debugf("Unblocking acquisition task");
	if (first_protocol_to_try == obd_proto_none){
		_startup_protocol = obd_proto_auto;
	} else {
		_startup_protocol = first_protocol_to_try;
	}
	if (use_default_config){
		_startup_protocol |= AUTODETECT_PIDS_MASK;
	}
}

void acquisition_add_channel(const acquisition_channel_t *channel){
	if (_channel_count < MAX_CHANNELS-1){
		memcpy(&_channel[_channel_count], channel, sizeof(acquisition_channel_t));
		_channel[_channel_count].failure_count = 0;
		debugf("Adding channel %ld type %d, PID mode %d, PID %02X, interval %ld",
				_channel_count,
				channel->channel_type,
				channel->pid_mode,
				channel->pid,
				(uint32_t)channel->interval);
		_channel_count++;
	} else {
		debugf("Too many channels!");
	}
}

static void autodetect_pids(void){
	static const uint8_t GET_SUPPORTED_PIDS_PIDS[][3] = {
			//PID, range min, range max
			{ 0x00, 0x01, 0x20 },
			{ 0x20, 0x21, 0x40 },
			{ 0x40, 0x41, 0x60 },
			{ 0x60, 0x61, 0x80 },
			{ 0x80, 0x81, 0xA0 },
			{ 0xA0, 0xA1, 0xC0 },
			{ 0xC0, 0xC1, 0xE0 },
	};

	for (uint32_t i = 0; i < sizeof(GET_SUPPORTED_PIDS_PIDS)/3; i++){

		bool read_okay = true;
		for (uint32_t retries = 0; retries < 3; retries++){
			obd_pid_response_t pid_response;
			int32_t status = obd_get_pid(pid_mode_01, GET_SUPPORTED_PIDS_PIDS[i][0], &pid_response);
			if (status > 0){
				log_pid(pid_mode_01,
						GET_SUPPORTED_PIDS_PIDS[i][0],
						pid_response.byte_a,
						pid_response.byte_b,
						pid_response.byte_c,
						pid_response.byte_d);

				uint32_t combined_value = //MSB holds the lowest supported PID
						pid_response.byte_a << 24 |
						pid_response.byte_b << 16 |
						pid_response.byte_c << 8  |
						pid_response.byte_d;
				debugf("PID %02X = %08X", GET_SUPPORTED_PIDS_PIDS[i][0], (unsigned int)combined_value);

				uint8_t current_pid = GET_SUPPORTED_PIDS_PIDS[i][1];
				for (uint32_t j = 0; j < 31; j++){ //last bit tells if a next "available PIDs" PID is available
					if (combined_value & (1<<31/*MSB*/)){ //32-bit MSB is set - PID is supported
						if (obd_pid_get_length(pid_mode_01, current_pid)){ //check if the PID is known at all

							uint8_t sampling_interval_seconds =
									obd_pid_get_default_sampling_interval_seconds(pid_mode_01, current_pid);
							//							debugf("PID %02X interval %d", current_pid, sampling_interval);
							if (sampling_interval_seconds != DONT_SAMPLE){

								acquisition_channel_t channel;
								channel.channel_type = logger_frame_pid;
								channel.pid_mode = pid_mode_01;
								channel.pid = current_pid;
								channel.interval = S_TO_TICKS(sampling_interval_seconds);
								channel.next_sample_timestamp = 0;
								acquisition_add_channel(&channel);
							}
						}
						current_pid++;
					} //end if 32-bit MSB is set
					combined_value = combined_value << 1;
				} //end of bit scanning loop

				if ((combined_value & (1<<31/*MSB*/)) == 0){
					//next "PIDs available" PID is not available
					debugf("End - no more supported PIDs");
					return;
				} else {
					debugf("More PIDs to detect");
				}

				break; //break out of the retry loop
			} else {//end if PID read success
				read_okay = false;
			}
		}//end of retry loop
		if (!read_okay){ //can't read "available PIDs" PID - no more PIDS to detect
			debugf("No more PIDs to detect");
			break;
		}
	}

	//add internal channels
	acquisition_channel_t channel;

	//add GPS logging every 5 seconds
	memset(&channel, 0, sizeof(channel));
	channel.channel_type = logger_frame_gps;
	channel.interval = S_TO_TICKS(5);
	channel.next_sample_timestamp = 0;
	acquisition_add_channel(&channel);

	//add internal diagnostics every 60 seconds
	memset(&channel, 0, sizeof(channel));
	channel.channel_type = logger_frame_internal_diagnostics;
	channel.interval = S_TO_TICKS(60);
	channel.next_sample_timestamp = 0;
	acquisition_add_channel(&channel);

	//add battry voltage every 10 seconds
	memset(&channel, 0, sizeof(channel));
	channel.channel_type = logger_frame_battery_voltage;
	channel.interval = S_TO_TICKS(10);
	channel.next_sample_timestamp = 0;
	acquisition_add_channel(&channel);
}
