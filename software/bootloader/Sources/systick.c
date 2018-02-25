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
#include <MKE06Z4.h>
#include "systick.h"

static volatile uint32_t systick_overflow_count;

void systick_init(void) {
          SysTick_Config(DEFAULT_SYSTEM_CLOCK/(1000/SYSTICK_PERIOD_MS));
}

inline uint32_t systick_get_overflow_count(void){
        return systick_overflow_count;
}

extern void SysTick_Handler(void);
void SysTick_Handler(void){ //ISR symbols are defined in startup code
	systick_overflow_count++;
}
