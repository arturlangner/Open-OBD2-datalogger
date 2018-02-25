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
#include "bitbang_uart.h"
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>
#include "gps_core.h"
#include "gps_uart.h"
#include <misc.h>
#include <MKE06Z4.h>
#include <stdbool.h>
#include <stdint.h>

#define DEBUG_ID DEBUG_ID_GPS_UART
#include <debug.h>

/* ------ hardware UART defines ------ */
static const UART_MemMapPtr _GPS_UART = UART0;
#define GPS_UART_IRQ_HANDLER UART0_IRQHandler
#define GPS_UART_IRQn UART0_IRQn
#define GPS_UART_CLOCK_ENABLE_MASK SIM_SCGC_UART0_MASK
/* ----------------------------------- */

#define GPS_BAUD 9600U
#define GPS_LINE_BUFFER_SIZE 100

typedef enum {
	gps_state_none = 0,
	gps_state_first_sentence_received = 1,
	gps_state_initialized = 2,
} gps_state_t;

static const char GPS_INIT_STRING[] = "$PMTK300,5000,0,0,0,0*18\r\n"; //sets update rate to 5000ms
static const char GPS_SLEEP_STRING[] = "$PMTK161,0*28\r\n";
static const char GPS_WAKE_STRING[] = "\r\n\r\n"; //anything will wake up the GPS module

static volatile uint8_t _rx_ringbuffer[256];
static volatile uint32_t _rx_ringbuffer_tail;
static uint32_t _rx_ringbuffer_head;

static const char *_tx_data_ptr;

bool ringbuffer_getc(char *target);

void gps_uart_init(void){
	//SIM_PINSEL0 &= ~SIM_PINSEL_UART0PS_MASK; //UART0 is on PTB0 (RX) and PTB1 (TX)
	//SIM_PINSEL0 = SIM_PINSEL_UART0PS_MASK; //UART0 is on PTA2 (RX) and PTA3 (TX)

	taskENTER_CRITICAL();
	SIM_SCGC |= GPS_UART_CLOCK_ENABLE_MASK; //enable clock to UART

	_GPS_UART->C2 = 0; //make sure everything is disabled before making changes

	_GPS_UART->C1 = 0; //no extra functions

	_GPS_UART->C1 |= UART_C1_LOOPS_MASK | UART_C1_RSRC_MASK; //workaround for mismatched footprint
	_GPS_UART->C3 = 0;

	//taken from Freescale demo driver
	/* Calculate baud settings */
	uint16_t u16Sbr = (((DEFAULT_BUS_CLOCK)>>4) + (GPS_BAUD>>1))/GPS_BAUD;

	/* Save off the current value of the UARTx_BDH except for the SBR field */
	uint8_t u8Temp = _GPS_UART->BDH & ~(UART_BDH_SBR_MASK);

	_GPS_UART->BDH = u8Temp |  UART_BDH_SBR(u16Sbr >> 8);
	_GPS_UART->BDL = (uint8_t)(u16Sbr & UART_BDL_SBR_MASK);

	_GPS_UART->C2 = UART_C2_TE_MASK /*enable transmitter*/
			| UART_C2_RE_MASK /*enable receiver*/
			| UART_C2_RIE_MASK /*enable receive interrupt*/;

	NVIC_SetPriority(GPS_UART_IRQn, 10);
	NVIC_EnableIRQ(GPS_UART_IRQn);

	taskEXIT_CRITICAL();

    bitbang_uart_init();

	debugf("GPS UART initialized");
}

void gps_uart_deinit(void){
	gps_uart_request_sleep(); //reduce power consumption to leave as much capacitance as possible for the SD card to flush

	_GPS_UART->C2 = 0; //disable UART
	NVIC_DisableIRQ(GPS_UART_IRQn);

	taskENTER_CRITICAL();
	SIM_SCGC &= ~GPS_UART_CLOCK_ENABLE_MASK; //disable clock to UART
	taskEXIT_CRITICAL();

	bitbang_uart_deinit(); //must be outside critical section - this will block until done
}

void gps_uart_task(void){
	static char line_buffer[GPS_LINE_BUFFER_SIZE];
	static uint32_t line_buffer_index = 0;
	char c = 0;
	while (ringbuffer_getc(&c)){
		if (unlikely(line_buffer_index > sizeof(line_buffer)-1)){ //overflow protection
			line_buffer_index = 0;
		}
		line_buffer[line_buffer_index] = c;
		line_buffer_index++;
		if (c == '\n'){ //whole line has arrived - fire callback
			line_buffer[line_buffer_index] = '\0';
			if (likely(gps_core_consume_nmea(line_buffer))){ //NMEA sentence was correct
				//disabled until next version of PCB - can't end data to the GPS
				static bool _gps_initialized = false;
				if (unlikely(_gps_initialized == false)){
					debugf("Lowering GPS refresh rate");
					_gps_initialized = true;
					bitbang_uart_transmit(sizeof(GPS_INIT_STRING)-1, (const uint8_t*)GPS_INIT_STRING, NULL/*no callback*/);
					//TODO: code for PCB v0.2 that uses hardware UART
//					_tx_data_ptr = GPS_INIT_STRING;
//					_GPS_UART->C2 |= UART_C2_TIE_MASK; //disable transmit interrupt
				}
			}
			line_buffer[line_buffer_index-2] = '\0';
			debugf("<%s>", line_buffer);

			line_buffer_index = 0;
			continue;
		} //end of if (c == '\n')
	}//end of while (ringbuffer_getc(&c))
}

void gps_uart_request_sleep(void){
	bitbang_uart_transmit(sizeof(GPS_SLEEP_STRING)-1, (const uint8_t*)GPS_SLEEP_STRING, NULL/*no callback*/);
}

void gps_uart_request_wake(void){
	bitbang_uart_transmit(sizeof(GPS_WAKE_STRING)-1, (const uint8_t*)GPS_WAKE_STRING, NULL/*no callback*/);
}

inline bool ringbuffer_getc(char *target){
	if (_rx_ringbuffer_tail != _rx_ringbuffer_head){
		*target = _rx_ringbuffer[_rx_ringbuffer_head];
		_rx_ringbuffer_head = (_rx_ringbuffer_head + 1) % sizeof(_rx_ringbuffer);
		return true;
	}
	return false;
}

extern void GPS_UART_IRQ_HANDLER(void);
void GPS_UART_IRQ_HANDLER(void){
	if (_GPS_UART->S1 & UART_S1_RDRF_MASK){ //new byte received
		_rx_ringbuffer[_rx_ringbuffer_tail] = _GPS_UART->D;
		_rx_ringbuffer_tail = (_rx_ringbuffer_tail + 1) % sizeof(_rx_ringbuffer);
	} else if (unlikely(_tx_data_ptr)){
		if (_GPS_UART->S1 & UART_S1_TDRE_MASK){ //transmit buffer empty
			if (_tx_data_ptr){
				if (*_tx_data_ptr){
					_GPS_UART->D = *_tx_data_ptr;
					_tx_data_ptr++;
				} else { //end of string
					_GPS_UART->C2 &= ~UART_C2_TIE_MASK; //disable transmit interrupt
					_tx_data_ptr = NULL;
				}
			}
		}
	}
}
