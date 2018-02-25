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
#include "crash_handler.h"
#include "misc.h"
#include <MKE06Z4.h>
#include <pins.h>
#include "power.h"
#include <spi0.h>

#if BOOTLOADER_BUILD == 0
#include "application_tasks.h"
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>
#include "gps_uart.h"
#include <obd/obd.h>
#include "timer.h"
#define DELAY_MS(x) vTaskDelay(pdMS_TO_TICKS(x))

#else //bootloader compatibility macros

#include "delay.h"
#define DELAY_MS(x) delay_ms(x)
#define taskENTER_CRITICAL() //empty macro
#define taskEXIT_CRITICAL() //empty macro

#endif


volatile bool GLOBAL_power_failure_flag = false;

static volatile bool _comparator_sense_voltage_drop; //used to switch between power off/on interrupt branch

void power_init(void){
	taskENTER_CRITICAL();
	SIM_SCGC |= SIM_SCGC_ACMP0_MASK; //enable clock to analog comparator
	taskEXIT_CRITICAL();

	_comparator_sense_voltage_drop = true;

	//Setup order is important - first DAC, then inputs, then comparator enable

	ACMP0->C1 = ACMP_C1_DACEN_MASK/*enable DAC*/ | ACMP_C1_DACREF_MASK/*ref = Vdda = 3,3V*/ | POWER_FAILURE_COMPARATOR_LEVEL;

	ACMP0->C0 = /*positive input is input 0*/ ACMP_C0_ACNSEL(3)/*negative input is DAC output*/;

	ACMP0->CS = ACMP_CS_ACE_MASK/*comparator enable*/ | ACMP_CS_ACIE_MASK /*interrupt enable*/ /*falling edge trigger*/;

	NVIC_SetPriority(ACMP0_IRQn, 1);
	NVIC_EnableIRQ(ACMP0_IRQn);
}

extern void ACMP0_IRQHandler(void);

#if BOOTLOADER_BUILD == 0
void ACMP0_IRQHandler(void){ //voltage rise or fall interrupt
	if (likely(_comparator_sense_voltage_drop)){ //this is a power failure
		LED1_OFF(); //all LEDs consume some power
		LED2_OFF();
		LED3_OFF();
		ACMP0->CS &= ~ACMP_CS_ACF_MASK; //clear comparator interrupt flag
		GLOBAL_power_failure_flag = true;
		vTaskSuspend(&acquisition_task_handle);
		portYIELD_FROM_ISR(true); //force switch to the storage task
	} else { //this is a wakeup
		strcpy(GLOBAL_news_from_the_grave, "!wkup");

		__DSB();

		NVIC_SystemReset(); //reset MCU
		__builtin_unreachable();
	}
}
#else //bootloader version
void ACMP0_IRQHandler(void){ //voltage rise interrupt
	asm("bkpt #0"); //TODO: remove after testing

	NVIC_SystemReset(); //reset MCU
	__builtin_unreachable();
}
#endif

__attribute__((noreturn)) void power_shutdown(void){
	adc_deinit();
#if BOOTLOADER_BUILD == 0
	vTaskSuspend(&acquisition_task_handle);
	gps_uart_deinit();

	obd_deinit_from_ISR();
	spi0_deinit_from_ISR();

	timer_deinit_from_ISR();
#else
	//TODO: deinit GPS
#endif

	//nice blink sequence
	LED1_ON();
	LED2_ON();
	LED3_ON();
	DELAY_MS(700);
	LED3_OFF();
	DELAY_MS(700);
	LED2_OFF();
	DELAY_MS(700);
	LED1_OFF();

	SD_POWER_OFF(); //placed after LED blink sequence to give the SD card as much time as possible to commit data

	//reconfigure analog comparator for wakeup
	ACMP0->CS = ACMP_CS_ACE_MASK/*comparator enable*/
			| ACMP_CS_ACIE_MASK /*interrupt enable*/
			| ACMP_CS_ACMOD(1)/*rising edge trigger*/;

	SCB->SCR = SCB_SCR_SLEEPDEEP_Msk; //enable deep sleep (Kinetis stop mode)

	SIM_SCGC &= ~SIM_SCGC_SWD_MASK; //disable SWD clock, the debugger will loose connection at this point

	strcpy(GLOBAL_news_from_the_grave, "!shdn");

	__DSB(); //make sure that the sleep deep bit is set before going into stop mode
	__WFI();

	//at this moment everything is stopped,
	//only the analog comparator is running and waiting for voltage rise

	//wait for a while after comparator wakeup interrupt occured
	//otherwise the breakpoint statement will generate a hard fault
	for (volatile uint32_t i = 0; i < 100; i++){
		__NOP();
	}

	//Should never reach here, because analog comparator ISR will do the reset first.
	//It however reaches this point when a J-Link is connected (and SWD module clock is not disabled).
	asm("bkpt #1"); //will reset the MCU anyway with no debugger connected
	__builtin_unreachable();
}

bool power_is_good(void){
	static uint32_t reading_below_threshold_count = 0;
#if BOOTLOADER_BUILD == 0
	static TickType_t last_check = 0;

	/* Voltage is measured every POWER_SHUTDOWN_VOLTAGE_SAMPLE_INTERVAL_ms + some jitter
	 * to avoid synchronizing with a cyclical load in the car (eg. wipers) that could
	 * wrongly trigger shutdown code.
	 *
	 * The maximum jitter added is 255ms.
	 */
	static uint8_t jitter_ms = 0;
	if (xTaskGetTickCount() - last_check > pdMS_TO_TICKS(POWER_SHUTDOWN_VOLTAGE_SAMPLE_INTERVAL_ms + jitter_ms)){
		jitter_ms++;

		last_check = xTaskGetTickCount();
#endif
		uint32_t voltage_code = adc_get_result();

		if (voltage_code < POWER_FAILURE_ADC_LEVEL){
			reading_below_threshold_count++;
		} else {
			reading_below_threshold_count = 0;
		}
#if BOOTLOADER_BUILD == 0
	}
#endif
	if (reading_below_threshold_count < POWER_SHUTDOWN_VOLTAGE_SAMPLE_BELOW_THRESHOLD_COUNT){
		return true;
	}
	return false;
}
