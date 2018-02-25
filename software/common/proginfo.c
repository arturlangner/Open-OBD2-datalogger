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
#include <proginfo.h>
#include "version.h"
const prog_info_no_length_t prog_info_no_length __attribute__((section(".prog_info_no_length"))) = {
                .program_type = SOFTWARE_TYPE,
                .version_major = SOFTWARE_VERSION_MAJOR,
                .version_minor = SOFTWARE_VERSION_MINOR,
};
