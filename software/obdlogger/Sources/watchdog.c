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
#include "watchdog.h"

void watchdog_init(void){

	WDOG->CNT = 0x20C5; //magic unlock
	WDOG->CNT = 0x28D9; //sequence

	WDOG->CS2 = WDOG_CS2_CLK(1)/*1kHz oscillator*/;
	WDOG->TOVAL = 30; //3 seconds

	//enable the watchdog, run in wait mode, don't run in debug mode, don't run in stop mode
	WDOG->CS1 = WDOG_CS1_EN_MASK | WDOG_CS1_WAIT_MASK;

}
void watchdog_kick(void){
	portENTER_CRITICAL();
	WDOG->CNT = 0x02A6;
	WDOG->CNT = 0x80B4;
	portEXIT_CRITICAL();
}
