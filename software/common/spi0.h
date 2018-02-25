/*
 * spi0.h
 *
 *  Created on: May 1, 2017
 *      Author: hexx
 */

/* This SPI driver is to be used from within an FreeRTOS task and ONLY ONE task. */

#ifndef SOURCES_SPI0_H_
#define SOURCES_SPI0_H_
#include <stdint.h>

void spi0_init(void);
void spi0_deinit_from_ISR(void);
void spi0_transfer_blocking(uint32_t length, uint8_t *data);
void spi0_transfer_blocking_no_readback(uint32_t length, const uint8_t *data);

void spi0_cs_high(void);
void spi0_cs_low(void);
void spi0_fast_clock(void);

#endif /* SOURCES_SPI0_H_ */
