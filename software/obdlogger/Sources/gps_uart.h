#ifndef SOURCES_GPS_UART_H_
#define SOURCES_GPS_UART_H_

void gps_uart_init(void);
void gps_uart_deinit(void);
void gps_uart_task(void);

void gps_uart_request_sleep(void);
void gps_uart_request_wake(void);

#endif /* SOURCES_GPS_UART_H_ */
