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
#if BOOTLOADER_BUILD == 0
#include <FreeRTOS/include/FreeRTOS.h>
#endif
#include <MKE06Z4.h>

void adc_init(void){
#if BOOTLOADER_BUILD == 0
	portENTER_CRITICAL();
#endif
	SIM_SCGC |= SIM_SCGC_ADC_MASK; //enable clock to ADC peripheral
#if BOOTLOADER_BUILD == 0
	portEXIT_CRITICAL();
#endif

	ADC->SC3 = ADC_SC3_ADLSMP_MASK/*long sample time*/
			| ADC_SC3_MODE(2)/*12-bit mode*/
			| ADC_SC3_ADICLK(1)/*bus clock/2*/
			| ADC_SC3_ADIV(3)/*divide clock by 8*/
			| ADC_SC3_ADLPC_MASK/*low power mode, max clock 4MHz*/;
	/* ADC clock must be within 0.4-8 MHz, bus clock is 20 MHz
	 * so divided by 8 gives 2.5 MHz which is within the datasheet limits. */

	ADC->APCTL1 = ADC_APCTL1_ADPC(1); //disable digital function of pin ADC0_SE0

	ADC->SC1 = ADC_SC1_ADCO_MASK/*continuous conversion*/
				| ADC_SC1_ADCH(0) /*select ADC0_SE0 input*/;
}

void adc_deinit(void){
	ADC->SC1 = ADC_SC1_ADCH_SHIFT << 0x1F; //disable ADC
#if BOOTLOADER_BUILD == 0
	portENTER_CRITICAL();
#endif
	SIM->SCGC &= ~SIM_SCGC_ADC_MASK; //disable clock to ADC
#if BOOTLOADER_BUILD == 0
	portEXIT_CRITICAL();
#endif
}

uint32_t adc_get_result(void){
	return ADC->R;
}
