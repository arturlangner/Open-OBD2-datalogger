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
#include <MKE06Z4.h>
#include "obd_uart.h"
#include "pins.h"

#define DEBUG_ID DEBUG_ID_OBD_UART
#include <debug.h>

#define K_LINE_BAUD 10400U

static TaskHandle_t _local_task_handle = NULL;

static const uint8_t *_tx_data_ptr;
static uint32_t _tx_bytes_to_send;

static uint8_t *_rx_data_ptr;
static volatile uint32_t _rx_desired_length;
static volatile uint32_t _rx_received_count;

void obd_uart_init_once(void){
	_local_task_handle = xTaskGetCurrentTaskHandle();
	portENTER_CRITICAL();

	K_LINE_INIT();

	SIM_SCGC |= K_LINE_UART_CLOCK_ENABLE_MASK; //enable clock to UART

	SIM_PINSEL1 &= SIM_PINSEL1_UART2PS_MASK; //TX PTD7, RX PTD6

	portEXIT_CRITICAL();
}

void obd_uart_init(void){
	portENTER_CRITICAL();

	K_LINE_UART->C2 = 0; //make sure everything is disabled before making changes
	K_LINE_UART->C1 = 0; //no extra functions

	K_LINE_UART->C3 = UART_C3_TXINV_MASK; //TX is inverted because the UART drives K-Line via NPN transistor
	//so pin high is K-Line low and vice-versa

	//taken from Freescale demo driver
	/* Calculate baud settings */
	uint16_t u16Sbr = (((DEFAULT_BUS_CLOCK)>>4) + (K_LINE_BAUD>>1))/K_LINE_BAUD;

	/* Save off the current value of the UARTx_BDH except for the SBR field */
	uint8_t u8Temp = K_LINE_UART->BDH & ~(UART_BDH_SBR_MASK);

	K_LINE_UART->BDH = u8Temp |  UART_BDH_SBR(u16Sbr >> 8);
	K_LINE_UART->BDL = (uint8_t)(u16Sbr & UART_BDL_SBR_MASK);

	NVIC_SetPriority(K_LINE_UART_IRQn, 5);
	NVIC_EnableIRQ(K_LINE_UART_IRQn);

	taskEXIT_CRITICAL();
	debugf("K-Line UART initialized");
}

void obd_uart_deinit(void){
	K_LINE_UART->C2 = 0; //disable UART, pins are now GPIO
	K_LINE_HIGH();
}

void obd_uart_deinit_from_ISR(void){ //used for power shutdown
	SIM_SCGC |= K_LINE_UART_CLOCK_ENABLE_MASK; //enable clock to UART in case it was never initialized before

	K_LINE_UART->C2 = 0; //disable UART, pins are now GPIO
	K_LINE_HIGH();

	SIM_SCGC &= ~K_LINE_UART_CLOCK_ENABLE_MASK; //disable clock to UART
}

void obd_uart_pin_high(void){
	K_LINE_HIGH();
}

void obd_uart_pin_low(void){
	K_LINE_LOW();
}

void obd_uart_send(const uint8_t *data, uint32_t length){
	_tx_bytes_to_send = length;
	_tx_data_ptr = data;

	__DSB(); //ensure that the variables were comitted to memory before enabling interrupts

	debugf("TX start");

	K_LINE_UART->C2 = UART_C2_TE_MASK /*enable transmitter*/
			| UART_C2_TIE_MASK /*enable transmitter ready interrupt*/;

	//now an interrupt will happen and start transmitting

	//wait for notification
	ulTaskNotifyTake(
			pdTRUE/*clear notification value when ready*/,
			pdMS_TO_TICKS(1000/*ms*/)); //wait up to 1s for transmission

	debugf("TX done");
}

uint32_t obd_uart_receive_frame(uint8_t *target_data, uint32_t timeout_ms){
	_rx_data_ptr = target_data;
	_rx_desired_length = 0; //length will be known after the first byte is received
	_rx_received_count = 0;

	__DSB(); //ensure that the variables were comitted to memory before enabling interrupts

	K_LINE_UART->C2 = UART_C2_RE_MASK /*enable receiver*/
			| UART_C2_RIE_MASK /*enable receive interrupt*/;

	ulTaskNotifyTake(
			pdTRUE/*clear notification value when ready*/,
			pdMS_TO_TICKS(timeout_ms));
	debugf("Received %ld bytes", _rx_received_count);

	if (_rx_received_count){
		if (obd_uart_verify_checksum(target_data, _rx_received_count)){
			return _rx_received_count;
		}
		return 0; //wrong frame
	}
	return 0; //no data received
}

bool obd_uart_verify_checksum(const uint8_t *data, uint32_t length){
	uint8_t checksum = 0;
	for (uint32_t i = 0; i < length - 1; i++){
		checksum += data[i];
	}
	if (data[length - 1] == checksum){
		debugf("Checksum okay %02X %02X", data[length - 1], checksum);
		return true;
	}
	debugf("Checksum NOT okay %02X %02X", data[length - 1], checksum);
	return false;
}

uint32_t obd_uart_receive(uint8_t *data, uint32_t desired_length, uint32_t timeout_ms){
	debugf("RX start, expected %ld, timeout %ld", desired_length, timeout_ms);
	_rx_data_ptr = data;
	_rx_desired_length = desired_length;
	_rx_received_count = 0;

	__DSB(); //ensure that the variables were comitted to memory before enabling interrupts

	K_LINE_UART->C2 = UART_C2_RE_MASK /*enable receiver*/
			| UART_C2_RIE_MASK /*enable receive interrupt*/;

	ulTaskNotifyTake(
			pdTRUE/*clear notification value when ready*/,
			pdMS_TO_TICKS(timeout_ms));
	debugf("Received %ld bytes", _rx_received_count);
	return _rx_received_count;

}

extern void K_LINE_UART_IRQ_HANDLER(void);
void K_LINE_UART_IRQ_HANDLER(void){

	//status register must be read to automatically clear flags when sending/receiving more bytes
	__attribute__((unused)) volatile uint8_t dummy_read = K_LINE_UART->S1;

	if (_tx_data_ptr){ //we are sending

		switch (_tx_bytes_to_send){
		case 0: //sending done - disable transmitter
			_tx_data_ptr = NULL;
			K_LINE_UART->C2 = 0; //disable UART (pins are now GPIO)
			K_LINE_HIGH();


			BaseType_t xHigherPriorityTaskWoken = pdFALSE;
			xTaskNotifyFromISR(_local_task_handle, pdTRUE, eSetValueWithOverwrite,
					&xHigherPriorityTaskWoken);
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			break;

		case 1: //last byte - wait for the transmit complete interrupt (not transmit buffer ready interrupt)
			K_LINE_UART->C2 = UART_C2_TE_MASK /*enable transmitter*/
			| UART_C2_TCIE_MASK /*enable transmit complete interrupt*/;

			__attribute__((fallthrough)); //transmit next byte! (attribute understood by GCC 7+)

		default: //transmit next byte
			K_LINE_UART->D = *_tx_data_ptr;
			_tx_data_ptr++;
			_tx_bytes_to_send--;
		}

	} else { //we are receiving
		if (_rx_desired_length){
			*_rx_data_ptr = K_LINE_UART->D;
			_rx_data_ptr++;
			_rx_received_count++;
			if (_rx_received_count == _rx_desired_length){ //all desired data has been received
				BaseType_t xHigherPriorityTaskWoken = pdFALSE;
				xTaskNotifyFromISR(_local_task_handle, pdTRUE, eSetValueWithOverwrite,
						&xHigherPriorityTaskWoken);
				portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
			}
		} else { //length of the frame is not known - parse first byte
			*_rx_data_ptr = K_LINE_UART->D;

			uint8_t length_byte = *_rx_data_ptr & 0x3F; //6 LSBs hold data field length
			_rx_desired_length = length_byte + 4 /*start byte, 2 type bytes, crc byte*/;

			_rx_data_ptr++;
			_rx_received_count++;
		}
	}
}
