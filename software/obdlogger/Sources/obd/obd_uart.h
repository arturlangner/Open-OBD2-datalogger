#ifndef SOURCES_OBD_OBD_UART_H_
#define SOURCES_OBD_OBD_UART_H_
#include <stdbool.h>
#include <stdint.h>

void obd_uart_init_once(void); //enables UART clock

void obd_uart_init(void);  //configures pin as UART
void obd_uart_deinit(void);//configures pin as GPIO
void obd_uart_deinit_from_ISR(void); //power down

void obd_uart_pin_high(void);
void obd_uart_pin_low(void);

void obd_uart_send(const uint8_t *data, uint32_t length);

uint32_t obd_uart_receive_frame(uint8_t *target_data, uint32_t timeout_ms); //returns total frame length

uint32_t obd_uart_receive(uint8_t *data, uint32_t desired_length, uint32_t timeout_ms);

bool obd_uart_verify_checksum(const uint8_t *data, uint32_t length);

#endif /* SOURCES_OBD_OBD_UART_H_ */
