#pragma once
#include <stdint.h>

typedef void (*bitbang_uart_tx_complete_callback)(void);

void bitbang_uart_init(void);
void bitbang_uart_deinit(void);

//data must be statically allocated, callback is delivered from ISR
void bitbang_uart_transmit(uint32_t length, const uint8_t *data, bitbang_uart_tx_complete_callback cb);
