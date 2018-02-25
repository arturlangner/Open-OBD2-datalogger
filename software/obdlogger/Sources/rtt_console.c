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
#include <crash_handler.h>
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>
#include "gps_uart.h"
#include "power.h"
#include "rtt_console.h"
#include <SEGGER/SEGGER_RTT.h>
#include <stdlib.h>
#include "spi0.h"
#include <string.h>

#if DEBUG_RTT_ENABLE == 1

#include "storage_task.h"

#define DEBUG_ID DEBUG_ID_CONSOLE
#include <debug.h>

static void _process_cmd_line(char *line);
static void _execute_cmd(uint32_t argc, char *argv[]);

void rtt_console_task(void){
	static uint8_t input_buffer[16];
	static uint32_t input_buffer_index = 0;

	if (input_buffer_index >= sizeof(input_buffer)-1){
		input_buffer_index = 0;
		debugf("ERROR *** OVERFLOW 1 ***");
	}
	uint32_t bytes_read;
	while ((bytes_read = SEGGER_RTT_ReadNoLock(0, input_buffer + input_buffer_index, 1)) == 1){
		if (*(input_buffer + input_buffer_index) == '\n'
				|| *(input_buffer + input_buffer_index) == '\r'
						|| *(input_buffer + input_buffer_index) == '!'){
			*(input_buffer + input_buffer_index) = '\0';
			_process_cmd_line((char*)input_buffer);
			input_buffer_index = 0;
			return; //don't process more commands
		}
		input_buffer_index += bytes_read;
	}
}

#define MAX_ARGUMENTS 5
static void _process_cmd_line(char *line){
	debugf("Line is <%s>", line);
	//tokenize the command string by spaces
	char* argv[MAX_ARGUMENTS];
	uint32_t argc = 1;
	uint32_t original_length = strlen(line);
	argv[0] = line;
	for (uint8_t i = 0; i < original_length; i++){
		if (line[i] == ' '){ //space separates command arguments
			line[i] = '\0';
			argv[argc] = line+i+1;
			argc++;
			if (argc > MAX_ARGUMENTS-1){
				debugf("TOO MANY ARGUMENTS");
				return;
			}
		}
	}
	_execute_cmd(argc, argv);
}

static void _execute_cmd(uint32_t argc, char *argv[]){
	if (argc == 1){
		if (argv[0][0] == 's'){
			debugf("Syncing log");
			storage_sync();
		} else if (argv[0][0] == '0'){
			debugf("SPI0 CS low");
			spi0_cs_low();
		} else if (argv[0][0] == '1'){
			debugf("SPI0 CS high");
			spi0_cs_high();
		} else if (argv[0][0] == 'a'){
			debugf("ADC = %ld (COMP thr %d, ADC thr %d)",
					adc_get_result(),
					POWER_FAILURE_COMPARATOR_LEVEL,
					POWER_FAILURE_ADC_LEVEL);
		} else if (argv[0][0] == 'z'){
			debugf("GPS sleep");
			gps_uart_request_sleep();
		} else if (argv[0][0] == 'Z'){
			debugf("GPS wake");
			gps_uart_request_wake();
		} else if (argv[0][0] == 'Q'){
			debugf("Forced power down!");
			extern void ACMP0_IRQHandler(void);
			ACMP0_IRQHandler();
		} else if (argv[0][0] == 'C'){
			strcpy(GLOBAL_news_from_the_grave, "!console");
			NVIC_SystemReset();
		} else if (argv[0][0] == 'H'){
			//provoke hard fault
			volatile uint8_t *a;
			a = 0;
			*a = 5; //kaboom!
		}
	} else if (argc == 3){
		if (argv[0][0] == 'd'){
			debug_enable_id(atoi(argv[1]), atoi(argv[2]));
		}
	}
}
#else
#define rtt_console_task() //empty macro
#endif
