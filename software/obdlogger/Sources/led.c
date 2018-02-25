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
#include "led.h"
#include <pins.h>
#include "timer.h"
#include <stdbool.h>

typedef enum {
	led_state_idle = 0,
	led_state_wait_for_off,
	led_state_off,
	led_state_on,
} led_blink_state_t;

static volatile led_blink_state_t _led_state[LED_COUNT];
static volatile bool _waiting_for_ISR;

static void led_timer_callback_from_ISR(void);
static void led_set(uint8_t led_index, bool led_on);

void led_blink_request(uint8_t led_index){
	if (_led_state[led_index] == led_state_idle){
		_led_state[led_index] = led_state_wait_for_off;
		if (_waiting_for_ISR == false){
			timer_request_timeout(LED_BLINK_TIME_ms, led_timer_callback_from_ISR);
		}
	}
}

static void led_timer_callback_from_ISR(void){
	_waiting_for_ISR = false;
	bool call_timer_again = false;
	for (uint32_t i = 0; i < LED_COUNT; i++){

		switch (_led_state[i]){

		case led_state_wait_for_off:
			_led_state[i] = led_state_off;
			led_set(i, false/*off*/);
			call_timer_again = true;
			break;

		case led_state_off:
			_led_state[i] = led_state_on;
			led_set(i, true/*on*/);
			call_timer_again = true;
			break;

		case led_state_on:
			_led_state[i] = led_state_idle;
			break;

		default:
			break;
		}
	}

	if (call_timer_again){
		timer_request_timeout(LED_BLINK_TIME_ms, led_timer_callback_from_ISR);
	}
}

static void led_set(uint8_t led_index, bool led_on){
	if (led_on){
		switch (led_index){
		default:
		case 1: LED1_ON(); break;
		case 2: LED2_ON(); break;
		case 3: LED3_ON(); break;
		}
	} else {
		switch (led_index){
		default:
		case 1: LED1_OFF(); break;
		case 2: LED2_OFF(); break;
		case 3: LED3_OFF(); break;
		}
	}
}
