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
#include "timer.h"
#include <MKE06Z4.h>

#define TIMER_IRQ_HANDLER PIT_CH0_IRQHandler
#define TIMER_IRQn PIT_CH0_IRQn
#define TIMER_PERIODIC_IRQn PIT_CH1_IRQn

static timer_callback_t _cb;

void timer_init(void){
	SIM_SCGC |= SIM_SCGC_PIT_MASK; //enable clock to the timer
	TIMER->MCR = PIT_MCR_FRZ_MASK; //enable timer, freeze in debug mode
	NVIC_SetPriority(TIMER_IRQn, 4);
	NVIC_EnableIRQ(TIMER_IRQn);

	NVIC_SetPriority(TIMER_PERIODIC_IRQn, 3);
	NVIC_EnableIRQ(TIMER_PERIODIC_IRQn);
}

void timer_deinit_from_ISR(void){
	NVIC_DisableIRQ(TIMER_PERIODIC_IRQn);
	NVIC_DisableIRQ(TIMER_IRQn);
	TIMER->MCR = PIT_MCR_MDIS_MASK; //disable module
	SIM_SCGC &= ~SIM_SCGC_PIT_MASK; //disable clock to the module
}

void timer_request_timeout(uint32_t timeout_ms, timer_callback_t cb){
	_cb = cb;

	TIMER->CHANNEL[0].LDVAL = (DEFAULT_BUS_CLOCK/*Hz*/ / 1000) * timeout_ms;
	TIMER->CHANNEL[0].TCTRL = PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK; //enable timer channel, enable interrupt
}

extern void TIMER_IRQ_HANDLER(void);
void TIMER_IRQ_HANDLER(void){
	TIMER->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK; //clear interrupt flag
	TIMER->CHANNEL[0].TCTRL = 0; //disable timer channel
	if (_cb){
		_cb();
	}
}

void timer_periodic_isr_enable(uint32_t period_ticks){
	TIMER->CHANNEL[1].LDVAL = period_ticks;
	TIMER->CHANNEL[1].TCTRL = PIT_TCTRL_TEN_MASK | PIT_TCTRL_TIE_MASK; //enable timer channel, enable interrupt
}

void timer_periodic_isr_disable(void){
	TIMER->CHANNEL[1].TCTRL = 0; //disable timer channel
}
