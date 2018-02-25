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
#include "diagnostics.h"
#include <FreeRTOS/include/FreeRTOS.h>

frame_diagnostics_t GLOBAL_diagnostics_frame = {
		.frame_type = logger_frame_internal_diagnostics, //those fields are never changed during runtime
		.timebase_hz = configTICK_RATE_HZ
};
