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
#include <log.h>
#include <delay.h>
#include <FatFS/ff.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <systick.h>
#include "version.h"

#define DEBUG_TERMINAL 0
#include <debug.h>

static FIL _log_file;
static bool _log_started = false;

static void log_string(char *s);

void log_start(void){

	if (!_log_started){
		_log_started = true;
		f_open(&_log_file, "obdlog/update.txt", FA_CREATE_ALWAYS | FA_WRITE);
		log_format("Log started %02d.%02d", SOFTWARE_VERSION_MAJOR, SOFTWARE_VERSION_MINOR);
	}

}

static void log_string(char *s){
	uint32_t len = strlen(s);
	uint32_t dummy;
	f_write(&_log_file, s, len, &dummy);
}

void log_entry_(uint32_t line, char *s){
	debugf("line %ld <%s>", line, s);
	char scratchpad[8];
	sprintf(scratchpad, "%05ld ", systick_get_overflow_count());
	log_string(scratchpad);
	log_string(s);
	scratchpad[0] = '\r';
	scratchpad[1] = '\n';
	scratchpad[2] = '\0';
	uint32_t dummy;
	f_write(&_log_file, scratchpad, 3, &dummy);
}

void log_close(void){
	static bool once = false;
	if (_log_started && (!once)){
		log_entry("End of log");
		f_close(&_log_file);
		delay_ms(300); //allow the SD card to save data before the application
	}                  //cycles power to the card and reinits
}
