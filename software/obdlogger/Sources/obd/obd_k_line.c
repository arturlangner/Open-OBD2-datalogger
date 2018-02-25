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
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>
#include "obd_k_line.h"
#include "obd_pids.h"
#include "obd_uart.h"
#include "pins.h"
#include <string.h>

#define DEBUG_ID DEBUG_ID_OBD_K_LINE
#include <debug.h>

#define K_LINE_RX_BUFFER_SIZE 16

#define KEEPALIVE_INTERVAL_ms 2000

typedef int32_t (*k_line_get_pid_func_t)(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response);

static int32_t obd_k_line_get_pid_kwp2000(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response);
static int32_t obd_k_line_get_pid_iso9141(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response);

static uint8_t _rx_buffer[K_LINE_RX_BUFFER_SIZE];
static k_line_get_pid_func_t _get_pid_func;
static uint8_t _ecu_destination_address;
static uint32_t _last_request_timestamp;

obd_protocol_t obd_k_line_init(obd_protocol_t first_protocol_to_try){
	obd_uart_init_once();
	L_LINE_INIT();

	uint32_t bytes_received = 0;

	if (first_protocol_to_try == obd_proto_kwp2000_slow){
		goto INIT_K_LINE_SLOW;
	}

	/* ------------- KWP 2000 fast init ------------- */
	debugf("KWP2000 fast init start");
	obd_uart_deinit();
	vTaskDelay(pdMS_TO_TICKS(25));
	K_LINE_LOW();
	L_LINE_LOW();
	vTaskDelay(pdMS_TO_TICKS(25));
	K_LINE_HIGH();
	L_LINE_HIGH();
	vTaskDelay(pdMS_TO_TICKS(25));

	obd_uart_init();

	const uint8_t kwp2000_fast_init_frame[] = { 0xC1, 0x33, 0xF1, 0x81, 0x66 };

	obd_uart_send(kwp2000_fast_init_frame, sizeof(kwp2000_fast_init_frame));

	bytes_received = obd_uart_receive_frame(_rx_buffer, 1200/*ms*/);

	debugf("bytes received %ld", bytes_received);

	if (bytes_received){
		for (uint32_t i = 0; i < bytes_received; i++){
			debugf("[%ld] = %02X", i, _rx_buffer[i]);
		}

		_ecu_destination_address = 0x33; //fast init destination address is always fixed
		_get_pid_func = obd_k_line_get_pid_kwp2000;
		debugf("KWP2000 fast init okay");
		return obd_proto_kwp2000_fast;
	}
	debugf("KWP2000 fast init failure");
	/* ------------- KWP 2000 fast init end --------- */

	/* ----- slow init KWP 2000 or ISO 9141-2 ------- */
	debugf("Slow init start");
	obd_uart_deinit();

	K_LINE_HIGH();
	L_LINE_HIGH();
	vTaskDelay(pdMS_TO_TICKS(2600)); //2,6s delay before attempting 5 baud init

	INIT_K_LINE_SLOW:

	K_LINE_LOW();
	L_LINE_LOW();
	vTaskDelay(pdMS_TO_TICKS(200));
	K_LINE_HIGH();
	L_LINE_HIGH();
	vTaskDelay(pdMS_TO_TICKS(400));
	K_LINE_LOW();
	L_LINE_LOW();
	vTaskDelay(pdMS_TO_TICKS(400));
	K_LINE_HIGH();
	L_LINE_HIGH();
	vTaskDelay(pdMS_TO_TICKS(400));
	K_LINE_LOW();
	L_LINE_LOW();
	vTaskDelay(pdMS_TO_TICKS(400));
	K_LINE_HIGH();
	L_LINE_HIGH();
	vTaskDelay(pdMS_TO_TICKS(227));

	obd_uart_init();
	bytes_received = obd_uart_receive(_rx_buffer, 3 /*desired length*/, 500 /*timeout ms*/);
	if (bytes_received == 3){
		if (_rx_buffer[0] == 0x55){ //first response byte must always be 0x55
			debugf("KB1 = %02X, KB2 = %02X", _rx_buffer[1], _rx_buffer[2]);

			//if both KB1 and KB2 are 0x08 or 0x94 then use ISO 9141-2
			bool use_iso9141 = false;
			if ( (_rx_buffer[1] == 0x08 && _rx_buffer[2] == 0x08) ||
					(_rx_buffer[1] == 0x94 && _rx_buffer[2] == 0x94) ){
				use_iso9141 = true;
			}
			//TODO: P2min is 0 for kb 0x94, 25ms for kb 0x08
			//FIXME: implement P2min

			uint8_t byte_to_send = ~_rx_buffer[2]; //reply with inverted KB2
			obd_uart_send(&byte_to_send, 1/*length*/);

			bytes_received = obd_uart_receive(_rx_buffer, 1 /*desired length*/, 100 /*timeout ms*/);
			if (bytes_received == 1){
				_ecu_destination_address = ~_rx_buffer[0];
				debugf("Destination address %02X", _ecu_destination_address);

				if (use_iso9141){
					debugf("ISO9141 slow init okay");
					_get_pid_func = obd_k_line_get_pid_iso9141;
					return obd_proto_iso9141;
				} else {
					debugf("KWP slow init okay");
					_get_pid_func = obd_k_line_get_pid_kwp2000;
					return obd_proto_kwp2000_slow;
				}
			}
		}
	}
	debugf("Slow init failure");
	/* ----- slow init KWP 2000 or ISO 9141-2 end --- */

	return obd_proto_none; //failed to detect K-Line protocol
}

void obd_k_line_deinit_from_ISR(void){
	L_LINE_HIGH();
	obd_uart_deinit_from_ISR();
}

void obd_k_line_task(void){
	if (xTaskGetTickCount() > _last_request_timestamp + pdMS_TO_TICKS(KEEPALIVE_INTERVAL_ms)){
		debugf("Keepalive request");
		static obd_pid_response_t dummy;
		obd_k_line_get_pid(pid_mode_01, 0x00/*available PIDs*/, &dummy);
	}
}

int32_t obd_k_line_get_pid(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response){
	if (_get_pid_func){

		_last_request_timestamp = xTaskGetTickCount();

		return _get_pid_func(mode, pid, target_response);
	}
	return OBD_PID_ERR;
}

static int32_t obd_k_line_get_pid_iso9141(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response){
	uint8_t pid_length = obd_pid_get_length(mode, pid);

	if (mode == pid_mode_01 || mode == pid_mode_02){
		uint8_t request_frame[] = { 0x68, 0x6A, 0xF1, mode, pid, 0/*CRC to be computed*/ };
		uint8_t checksum = 0;
		for (uint32_t i = 0; i < sizeof(request_frame) - 1; i++){
			checksum += request_frame[i];
		}
		request_frame[sizeof(request_frame)-1] = checksum;

		obd_uart_send(request_frame, sizeof(request_frame));

		uint32_t bytes_received = obd_uart_receive(_rx_buffer, pid_length + 6/*header,framing etc.*/ , 200/*ms*/);

		debugf("bytes received %ld", bytes_received);
		if (bytes_received){

			for (uint32_t i = 0; i < bytes_received; i++){
				debugf("[%ld] = %02X", i, _rx_buffer[i]);
			}

			if (obd_uart_verify_checksum(_rx_buffer, bytes_received)){

				target_response->byte_a = 0;
				target_response->byte_b = 0;
				target_response->byte_c = 0;
				target_response->byte_d = 0;
				memcpy((uint8_t*)target_response, _rx_buffer+5/*skip headers etc.*/, pid_length);

				debugf("Response PID length %d %02X%02X%02X%02X",
						pid_length,
						target_response->byte_a,
						target_response->byte_b,
						target_response->byte_c,
						target_response->byte_d);
				return pid_length;
			}
		}
	} else {
		debugf("Unsupported PID mode");
	}
	debugf("Get PID failure");
	return OBD_PID_ERR;
}

static int32_t obd_k_line_get_pid_kwp2000(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response){
	if (mode == pid_mode_01 || mode == pid_mode_02){

		uint8_t request_frame[6] = { 0xC2, _ecu_destination_address, 0xF1, mode, pid };
		uint8_t checksum = 0;
		for (uint32_t i = 0; i < sizeof(request_frame) - 1; i++){
			checksum += request_frame[i];
		}
		request_frame[5] = checksum;

		obd_uart_send(request_frame, sizeof(request_frame));

		uint32_t bytes_received = obd_uart_receive_frame(_rx_buffer, 1200/*ms*/);

		debugf("bytes received %ld", bytes_received);
		if (bytes_received){

			for (uint32_t i = 0; i < bytes_received; i++){
				debugf("[%ld] = %02X", i, _rx_buffer[i]);
			}

			uint32_t pid_length = bytes_received - 4/*start byte, 2 type bytes, crc byte*/;

			target_response->byte_a = 0;
			target_response->byte_b = 0;
			target_response->byte_c = 0;
			target_response->byte_d = 0;
			memcpy((uint8_t*)target_response, _rx_buffer+5, pid_length);

			debugf("Response PID length %ld %02X%02X%02X%02X",
					pid_length,
					target_response->byte_a,
					target_response->byte_b,
					target_response->byte_c,
					target_response->byte_d);
			return pid_length;
		}
	} else {
		debugf("Unsupported PID mode");
	}
	debugf("Get PID failure");
	return OBD_PID_ERR;
}
