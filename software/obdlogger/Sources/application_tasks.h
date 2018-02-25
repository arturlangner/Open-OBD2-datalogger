#ifndef SOURCES_APPLICATION_TASKS_H_
#define SOURCES_APPLICATION_TASKS_H_
#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>

//sizes in bytes
#define STACK_SIZE_ACQUISITION_TASK 1024
#define STACK_SIZE_STORAGE_TASK 1024

extern StaticTask_t acquisition_task_handle;
extern StaticTask_t storage_task_handle;

extern StackType_t STACK_ACQUISITION_TASK[STACK_BYTES_TO_WORDS(STACK_SIZE_ACQUISITION_TASK)];
extern StackType_t STACK_STORAGE_TASK[STACK_BYTES_TO_WORDS(STACK_SIZE_STORAGE_TASK)];

#endif /* SOURCES_APPLICATION_TASKS_H_ */
