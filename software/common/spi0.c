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
#if BOOTLOADER_BUILD == 0
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>
#endif
#include "pins.h"
#include "spi0.h"
#include <core_cm0plus.h>
#include <stdbool.h>

//#define DEBUG_TERMINAL 0
#include <debug.h>

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

static const SPI_MemMapPtr _spi = SPI0;

#if BOOTLOADER_BUILD == 0
static TaskHandle_t xTaskToNotify = NULL;
#else
static volatile bool _transfer_complete_flag;
#endif

static uint8_t *_data_ptr;
static uint8_t *_data_end_ptr;
static bool _write_only;

void spi0_init(void){
#if BOOTLOADER_BUILD == 0
	taskENTER_CRITICAL();
#endif
	//SIM_SOPT0 &= ~SIM_SOPT0_NMIE_MASK; //disable NMI pin, it will be used as MISO
	SPI0_PINS_INIT();

	SIM_SCGC |= SIM_SCGC_SPI0_MASK; //enable clock for SPI0

	spi0_cs_high();
#if BOOTLOADER_BUILD == 0
	taskEXIT_CRITICAL();
#endif

	_spi->C2 = 0;

	//SLOWEST SPI clock for SD card initialization 4,88kHz @ 20MHz bus clock
	_spi->BR = SPI_BR_SPPR(8) | SPI_BR_SPR(7);

	NVIC_SetPriority(SPI0_IRQn, 5);
	NVIC_EnableIRQ(SPI0_IRQn);
#if BOOTLOADER_BUILD == 0
	xTaskToNotify = xTaskGetCurrentTaskHandle();
#else
	_transfer_complete_flag = false;
#endif
	debugf("SPI init done");
}

void spi0_deinit_from_ISR(void){
	SIM_SCGC |= SIM_SCGC_SPI0_MASK; //enable clock for SPI0 in case it was not yet initialized
	_spi->C1 = 0;
	SIM_SCGC &= ~SIM_SCGC_SPI0_MASK; //disable clock for SPI0
	SPI0_PINS_DEINIT();
}

void spi0_transfer_blocking(uint32_t length, uint8_t *data){
#if BOOTLOADER_BUILD == 1
	_transfer_complete_flag = false;
#endif
	debugf("len=%ld", length);
	_spi->C1 = SPI_C1_SPE_MASK  /*enable SPI module*/
			| SPI_C1_SPIE_MASK /*enable SPI receive complete interrupt*/
			| SPI_C1_MSTR_MASK;/*master mode*/

	_spi->C2 = 0;

	_data_ptr = data;
	_data_end_ptr = data + length;
	_write_only = false;

	volatile uint32_t status __attribute__((unused)) = _spi->S;

	_spi->D = *_data_ptr; //transmit first byte
#if BOOTLOADER_BUILD == 0
	//at this point ISR will be doing the job, the task will block
	ulTaskNotifyTake(pdTRUE/*clear notification value when ready*/,	portMAX_DELAY/*wait indefinitely*/);
#else
	while (!_transfer_complete_flag){
		//busy wait
	}
#endif
}

void spi0_transfer_blocking_no_readback(uint32_t length, const uint8_t *data){
#if BOOTLOADER_BUILD == 1
	_transfer_complete_flag = false;
#endif
	debugf("len=%ld", length);
	_spi->C1 = SPI_C1_SPE_MASK  /*enable SPI module*/
			| SPI_C1_SPIE_MASK /*enable SPI receive complete interrupt*/
			| SPI_C1_MSTR_MASK; /*master mode*/

	_data_ptr = (uint8_t*)data; //cast this const pointer to "normal" pointer to keep commonality in the ISR
	_data_end_ptr = ((uint8_t*)data) + length;
	_write_only = true;

	volatile uint32_t dummy_read __attribute__((unused)) = _spi->S; //dummy read to clear interrupt flags

	_spi->D = *_data_ptr; //transmit first byte
#if BOOTLOADER_BUILD == 0
	//at this point ISR will be doing the job, the task will block
	ulTaskNotifyTake(pdTRUE/*clear notification value when ready*/,	portMAX_DELAY/*wait indefinitely*/);
#else
	while (!_transfer_complete_flag){
		//busy wait
	}
#endif
}

inline void spi0_cs_high(void){
	debugf("CS HIGH");
	SPI0_CS_HIGH();
}

inline void spi0_cs_low(void){
	debugf("CS LOW");
	SPI0_CS_LOW();
}

extern void SPI0_IRQHandler(void);
void SPI0_IRQHandler(void){
	volatile uint32_t dummy_read __attribute__((unused)) = _spi->S; //dummy read to clear interrupt flags

	if (_write_only == false){
		*_data_ptr = _spi->D; //store incoming byte
	} else {
		volatile uint32_t dummy_read2 __attribute__((unused)) = _spi->D;
	}
	_data_ptr++;

	if (likely(_data_ptr != _data_end_ptr)){
		_spi->D = *_data_ptr; //transmit next byte
	} else { //end of transfer
		//disable interrupt
		_spi->C1 = SPI_C1_SPE_MASK  /*enable SPI module*/
				| SPI_C1_MSTR_MASK; /*master mode*/

		//unblock task
#if BOOTLOADER_BUILD == 0
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xTaskNotifyFromISR(xTaskToNotify, pdTRUE, eSetValueWithOverwrite,&xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
#else
		_transfer_complete_flag = true;
#endif
	}
}

void spi0_fast_clock(void){
#ifdef BOARD_EVK
	//do nothing - keep using slow clock
#else
//	_spi->BR = SPI_BR_SPPR(1); //slowest possible clock
	_spi->BR = SPI_BR_SPPR(5) | SPI_BR_SPR(6); //62,5kHz @ 20MHz bus clock
#endif
}
