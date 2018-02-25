#ifndef DELAY_H_
#define DELAY_H_
#include <stdint.h>
#include "systick.h" //has to be here to access SYSTICK_PERIOD_MS

void delay_systicks(uint32_t n);


#define MS_TO_SYSTICKS(n) (n/SYSTICK_PERIOD_MS)

#define delay_ms(n) delay_systicks(MS_TO_SYSTICKS(n))

#endif /* DELAY_H_ */
