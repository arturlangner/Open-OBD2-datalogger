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
#include "diagnostics.h"
#include "led.h"
#include "logger_core.h"
#include "obd.h"
#include "obd_can.h"
#include "obd_k_line.h"
#include "pins.h"

#define DEBUG_ID DEBUG_ID_OBD
#include <debug.h>

#define MAX_PID_FAILURES 5

typedef int32_t (*obd_get_pid_internal_func_t)(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response);
typedef void (*obd_phy_subtask_t)(void);

static obd_get_pid_internal_func_t _obd_get_pid_internal_func;
static obd_phy_subtask_t _obd_phy_subtask;

#define OBD_INIT_TEST_PID 0x00 //PID used as for a test read, 0x00 = available PIDs 01-20

void obd_init(obd_protocol_t first_protocol_to_try){
	debugf("OBD initialization");
	obd_pid_response_t response;

	switch (first_protocol_to_try){ //speeds up initialization
	case obd_proto_iso9141: goto INIT_PROTO_K_LINE; break;
	case obd_proto_kwp2000_slow: goto INIT_PROTO_K_LINE; break;
	case obd_proto_kwp2000_fast: goto INIT_PROTO_K_LINE; break;
	case obd_proto_can_11b_500kbps: goto INIT_PROTO_CAN_11b_500kbit; break;
	case obd_proto_can_11b_250kbps: goto INIT_PROTO_CAN_11b_250kbit; break;
	case obd_proto_can_29b_500kbps: goto INIT_PROTO_CAN_29b_500kbit; break;
	case obd_proto_can_29b_250kbps: goto INIT_PROTO_CAN_29b_250kbit; break;
	default: break;
	}

	obd_protocol_t detected_protocol = obd_proto_none;

	while (1){ //init loop
		int32_t status;

		INIT_PROTO_CAN_11b_500kbit:
		debugf("Attempting init CAN 500kbaud standard id");
		obd_can_init(can_speed_500kbaud, CAN_STANDARD_ID);
		status = obd_can_get_pid(pid_mode_01, OBD_INIT_TEST_PID, &response);
		if (status > 0){
			debugf("Init okay - CAN 500kbaud standard id");
			_obd_get_pid_internal_func = obd_can_get_pid;
			_obd_phy_subtask = obd_can_task;
			detected_protocol = obd_proto_can_11b_500kbps;
			break;
		} else {
			debugf("Init failure");
		}

		INIT_PROTO_CAN_11b_250kbit:
		debugf("Attempting init CAN 250kbaud standard id");
		obd_can_init(can_speed_250kbaud, CAN_STANDARD_ID);
		status = obd_can_get_pid(pid_mode_01, OBD_INIT_TEST_PID, &response);
		if (status > 0){
			debugf("Init okay - CAN 250kbaud standard id");
			_obd_get_pid_internal_func = obd_can_get_pid;
			_obd_phy_subtask = obd_can_task;
			detected_protocol = obd_proto_can_11b_250kbps;
			break;
		} else {
			debugf("Init failure");
		}

		INIT_PROTO_CAN_29b_500kbit:
		debugf("Attempting init CAN 500kbaud extended id");
		obd_can_init(can_speed_500kbaud, CAN_EXTENDED_ID);
		status = obd_can_get_pid(pid_mode_01, OBD_INIT_TEST_PID, &response);
		if (status > 0){
			debugf("Init okay - CAN 500kbaud extended id");
			_obd_get_pid_internal_func = obd_can_get_pid;
			_obd_phy_subtask = obd_can_task;
			detected_protocol = obd_proto_can_29b_500kbps;
			break;
		} else {
			debugf("Init failure");
		}

		INIT_PROTO_CAN_29b_250kbit:
		debugf("Attempting init CAN 250kbaud extended id");
		obd_can_init(can_speed_250kbaud, CAN_EXTENDED_ID);
		status = obd_can_get_pid(pid_mode_01, OBD_INIT_TEST_PID, &response);
		if (status > 0){
			debugf("Init okay - CAN 250kbaud extended id");
			_obd_get_pid_internal_func = obd_can_get_pid;
			_obd_phy_subtask = obd_can_task;
			detected_protocol = obd_proto_can_29b_250kbps;
			break;
		} else {
			debugf("Init failure");
		}

		INIT_PROTO_K_LINE:
		detected_protocol = obd_k_line_init(first_protocol_to_try);
		if (detected_protocol != obd_proto_none){
			debugf("Init okay - K-Line");
			_obd_get_pid_internal_func = obd_k_line_get_pid;
			_obd_phy_subtask = obd_k_line_task;
			break;
		}

		vTaskDelay(pdMS_TO_TICKS(1000)); //wait at least one second for another initialization attempt

	} //end of initialization loop
	LED2_ON();

	if (first_protocol_to_try != detected_protocol){
		//save the detected protocol to speed up initialization at next power on
		log_detected_protocol(detected_protocol);
	}
}

void obd_deinit_from_ISR(void){
	obd_can_deinit();
	obd_k_line_deinit_from_ISR();
}

void obd_task(void){
	if (_obd_phy_subtask){
		_obd_phy_subtask();
	}
}

int32_t obd_get_pid(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response){
	led_blink_request(LED_OBD);

	int32_t status = _obd_get_pid_internal_func(mode, pid, target_response);
	if (status < 1){ //reading PID failed
		debugf("PID %02X read failure %d, status %ld", pid, GLOBAL_diagnostics_frame.pid_get_failures, status);
		GLOBAL_diagnostics_frame.pid_get_failures++;
		if (GLOBAL_diagnostics_frame.pid_get_failures > MAX_PID_FAILURES){
			GLOBAL_diagnostics_frame.pid_get_failures = 0;
			LED2_OFF();
			obd_init(obd_proto_auto);
		}
	} else {
		GLOBAL_diagnostics_frame.pid_get_failures = 0;
	}
	return status;
}
