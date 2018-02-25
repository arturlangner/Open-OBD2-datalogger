#ifndef SOURCES_SYSTICK_H_
#define SOURCES_SYSTICK_H_

#include <stdint.h>

#define SYSTICK_PERIOD_MS 10

void systick_init(void);

uint32_t systick_get_overflow_count(void); //this function returns how many has the 10ms interrupt occurred
                                           //IT WILL OVERFLOW EVERY 497 DAYS so don't depend on the absolute value!

#endif /* SOURCES_SYSTICK_H_ */
