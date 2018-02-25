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
#include "acquisition_task.h"
#include "application_tasks.h"
#include "crash_handler.h"
#include <debug.h>
#include "MKE06Z4.h"
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/queue.h>
#include <FreeRTOS/include/task.h>
#include <FreeRTOS/include/semphr.h>
#include "SEGGER/SEGGER_RTT.h"
#include "storage_task.h"
#include "pins.h"
#include "timer.h"
#include "watchdog.h"

void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);

int main(void){

	SEGGER_RTT_Init();
	SEGGER_RTT_printf(0, "hello\n");
#ifdef USE_SYSTEMVIEW
	SEGGER_SYSVIEW_Conf();
#endif

	crash_handler_init();

	LED_INIT();

	watchdog_init();

    debug_init();

    timer_init();

    xTaskCreateStatic(storage_task,            //task function
    				  "storage task",            //task logical name
					  STACK_BYTES_TO_WORDS(STACK_SIZE_STORAGE_TASK),  //size of stack
					  NULL,                  //no extra parameters
					  1,                     //priority (0 is lowest - idle task)
					  STACK_STORAGE_TASK,          //stack pointer
					  &storage_task_handle);       //pointer to StaticTask_t

    xTaskCreateStatic(acquisition_task,            //task function
    				  "acquisition task",            //task logical name
					  STACK_BYTES_TO_WORDS(STACK_SIZE_ACQUISITION_TASK),  //size of stack
					  NULL,                  //no extra parameters
					  1,                     //priority (0 is lowest - idle task)
					  STACK_ACQUISITION_TASK,          //stack pointer
					  &acquisition_task_handle);       //pointer to StaticTask_t

    vTaskStartScheduler(); //never returns, enables interrupts

    return 0;
}

void vApplicationIdleHook(void){
	watchdog_kick();
	//if there is nothing to do - CPU core sleep
	__WFI();
}

void vApplicationStackOverflowHook(TaskHandle_t xTask __attribute__((unused)), signed char *pcTaskName){
	snprintf(GLOBAL_news_from_the_grave, sizeof(GLOBAL_news_from_the_grave), "!stk<%s>", pcTaskName);
	__DSB();
	asm("bkpt #1");
}

#if configSUPPORT_STATIC_ALLOCATION
/* static memory allocation for the IDLE task */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE]; //StackType_t is uint32_t!

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize);

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = xIdleStack;
  *pulIdleTaskStackSize = STACK_BYTES_TO_WORDS(sizeof(xIdleStack));
}
#endif

#if configSUPPORT_STATIC_ALLOCATION && configUSE_TIMERS
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[256/sizeof(StackType_t)];

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize);
/* If static allocation is supported then the application must provide the
   following callback function - which enables the application to optionally
   provide the memory that will be used by the timer task as the task's stack
   and TCB. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize) {
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = sizeof(xTimerStack);
}
#endif
