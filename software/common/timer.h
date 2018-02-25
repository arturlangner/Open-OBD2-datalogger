#pragma once
#include <stdint.h>

#define TIMER PIT

typedef void (*timer_callback_t)(void);

void timer_init(void); //call before RTOS is started
void timer_deinit_from_ISR(void); //call to save power, RTOS-unsafe
void timer_request_timeout(uint32_t timeout_ms, timer_callback_t cb);

#define TIMER_PERIODIC_IRQ_HANDLER PIT_CH1_IRQHandler
void timer_periodic_isr_enable(uint32_t period_ticks);
void timer_periodic_isr_disable(void);
// void TIMER_PERIODIC_IRQ_HANDLER(void) must be declared in another module!

#define timer_periodic_isr_clear() TIMER->CHANNEL[1].TFLG = PIT_TFLG_TIF_MASK
