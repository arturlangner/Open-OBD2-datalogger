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
#include <MKE06Z4.h>
#include <pins.h>
#include "timer.h"

/* This module uses the programmable interrupt timer (PIT)
 * to bit-bang the transmit side of a UART.
 *
 * PCB v0.1 has a bug - TX and RX GPS lines are crossed (wrong NXP library part),
 * the external TX line (which should be RX) can be wired internally to RX and
 * hardware uart, but the RX line (which should be TX) can not, so reception from the
 * GPS is handled by the hardware UART, but transmission to the GPS (to lower the refresh
 * rate and switch it to low-power state) is done by this module.
 */

#define BITBANG_BAUD 9600

typedef enum {
	uart_tx_state_start_bit,
	uart_tx_state_bit_0,
	uart_tx_state_bit_1,
	uart_tx_state_bit_2,
	uart_tx_state_bit_3,
	uart_tx_state_bit_4,
	uart_tx_state_bit_5,
	uart_tx_state_bit_6,
	uart_tx_state_bit_7,
	uart_tx_state_stop_bit,
	uart_tx_state_idle,
} uart_tx_fsm_t;

static volatile uint32_t _bytes_to_send;
static const uint8_t *_data_ptr;
static uint8_t _byte;
static uart_tx_fsm_t _fsm;
static bitbang_uart_tx_complete_callback _tx_complete_callback;

void bitbang_uart_init(void){
	UART_BITBANG_TX_INIT();
}

void bitbang_uart_deinit(void){
	while (_bytes_to_send){
		//wait until everything has been sent
	}
	UART_BITBANG_TX_LOW();
}

void bitbang_uart_transmit(uint32_t length, const uint8_t *data, bitbang_uart_tx_complete_callback cb){

	while (_bytes_to_send){} //previous data is still being transmitted - wait

	_bytes_to_send = length - 1;
	_data_ptr = data;
	_byte = *_data_ptr;
	_fsm = uart_tx_state_start_bit;
	_tx_complete_callback = cb;
	__DMB(); //ensure all data is in the memory before enabling interrupts
	timer_periodic_isr_enable(DEFAULT_BUS_CLOCK/*Hz*/ / BITBANG_BAUD);
}

extern void TIMER_PERIODIC_IRQ_HANDLER(void);
void TIMER_PERIODIC_IRQ_HANDLER(void){
	timer_periodic_isr_clear();

	switch (_fsm){
	case uart_tx_state_start_bit:
		UART_BITBANG_TX_LOW();
		_fsm = uart_tx_state_bit_0;
		break;

	case uart_tx_state_bit_0 ... uart_tx_state_bit_7:
		if (_byte & 0x01){ //send LSB first
			UART_BITBANG_TX_HIGH();
		} else {
			UART_BITBANG_TX_LOW();
		}
		_byte = _byte >> 1;
		_fsm++;
		break;

	case uart_tx_state_stop_bit:
		UART_BITBANG_TX_HIGH();
		_fsm = uart_tx_state_idle;
		break;

	case uart_tx_state_idle:
		if (_bytes_to_send){
			_data_ptr++;
			_byte = *_data_ptr;
			_fsm = uart_tx_state_start_bit;
			_bytes_to_send--;
		} else {
			timer_periodic_isr_disable();
			if (_tx_complete_callback){
				_tx_complete_callback();
			}
		}
		break;
	}
}
