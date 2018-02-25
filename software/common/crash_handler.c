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
#include "crash_handler.h"
#include <MKE06Z4.h>
#include <stdio.h>

//as seen on TV... https://mcuoneclipse.com/2012/11/24/debugging-hard-faults-on-arm-cortex-m/

#if BOOTLOADER_BUILD == 1
#include <pins.h>
#endif

char GLOBAL_news_from_the_grave[64] __attribute__ ((section (".news_from_the_grave,\"aw\",%nobits @")));;
static char _news_from_the_grave_copy[64];

void crash_handler_init(void){
	if (GLOBAL_news_from_the_grave[0] == '!'){
		GLOBAL_news_from_the_grave[sizeof(GLOBAL_news_from_the_grave)-1] = '\0';
		strcpy(_news_from_the_grave_copy, GLOBAL_news_from_the_grave);
	} else {
		memset(_news_from_the_grave_copy, 0, sizeof(_news_from_the_grave_copy));
	}
	memset(GLOBAL_news_from_the_grave, 0, sizeof(GLOBAL_news_from_the_grave));
}

char* crash_handler_get_info(void){
	return _news_from_the_grave_copy;
}

void HardFault_HandlerC(unsigned long *hardfault_args);
/**
 * HardFaultHandler_C:
 * This is called from the HardFault_HandlerAsm with a pointer the Fault stack
 * as the parameter. We can then read the values from the stack and place them
 * into local variables for ease of reading.
 * We then read the various Fault Status and Address Registers to help decode
 * cause of the fault.
 * The function ends with a BKPT instruction to force control back into the debugger
 */
void HardFault_HandlerC(unsigned long *hardfault_args){

        /* Attribute unused is to silence compiler warnings,
         * the variables are placed here, so they can be inspected
         * by the debugger.
         */
        volatile unsigned long __attribute__((unused)) stacked_r0;
        volatile unsigned long __attribute__((unused)) stacked_r1;
        volatile unsigned long __attribute__((unused)) stacked_r2;
        volatile unsigned long __attribute__((unused)) stacked_r3;
        volatile unsigned long __attribute__((unused)) stacked_r12;
        volatile unsigned long __attribute__((unused)) stacked_lr;
        volatile unsigned long __attribute__((unused)) stacked_pc;
        volatile unsigned long __attribute__((unused)) stacked_psr;

        stacked_r0 = ((unsigned long)hardfault_args[0]) ;
        stacked_r1 = ((unsigned long)hardfault_args[1]) ;
        stacked_r2 = ((unsigned long)hardfault_args[2]) ;
        stacked_r3 = ((unsigned long)hardfault_args[3]) ;
        stacked_r12 = ((unsigned long)hardfault_args[4]) ;
        stacked_lr = ((unsigned long)hardfault_args[5]) ;
        stacked_pc = ((unsigned long)hardfault_args[6]) ;
        stacked_psr = ((unsigned long)hardfault_args[7]) ;

#if BOOTLOADER_BUILD == 1
        LED1_ON();
        LED2_OFF();
        LED3_ON();
#endif

        GLOBAL_news_from_the_grave[0] = '!';
        sprintf(GLOBAL_news_from_the_grave, "!hf,%lX,", stacked_pc);
        __DSB(); //make sure all data is really written into the memory before doing a reset

        __asm("BKPT #0\n") ; //Break into the debugger or reset
}

extern void HardFault_Handler(void);

__attribute__((naked)) void HardFault_Handler(void)
{
        __asm volatile (
                        " movs r0,#4       \n"
                        " movs r1, lr      \n"
                        " tst r0, r1       \n"
                        " beq _MSP         \n"
                        " mrs r0, psp      \n"
                        " b _HALT          \n"
                        "_MSP:               \n"
                        " mrs r0, msp      \n"
                        "_HALT:              \n"
                        " ldr r1,[r0,#20]  \n"
                        " b HardFault_HandlerC \n"
                        " bkpt #0          \n"
        );
}
