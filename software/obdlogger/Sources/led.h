#ifndef SOURCES_LED_H_
#define SOURCES_LED_H_
#include <stdint.h>

#define LED_COUNT 3
#define LED_BLINK_TIME_ms 150

#define LED_SD_CARD 1
#define LED_OBD 2
#define LED_GPS 3

void led_blink_request(uint8_t led_index); //index can be: 1,2,3

#endif /* SOURCES_LED_H_ */
