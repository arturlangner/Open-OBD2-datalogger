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
#include "keys.h"

const uint8_t authentication_key[16] = {
		0xd7, 0x60, 0x1f, 0x5d,
		0x43, 0x24, 0x21, 0x8d,
		0x91, 0xbc, 0xaf, 0xb1,
		0x59, 0xd3, 0x18, 0xfd,
};

const uint8_t encryption_key[16] = {
		0xb9, 0x08, 0xf4, 0x2c,
		0x02, 0x4d, 0x13, 0xa1,
		0x35, 0x62, 0x6f, 0xef,
		0xf4, 0xca, 0x05, 0xdb,
};

const uint8_t encryption_iv[16] = {
		0xed, 0xa0, 0x9c, 0x22,
		0xc2, 0x63, 0xe8, 0xf2,
		0x97, 0xf8, 0x20, 0xb8,
		0x3f, 0x67, 0x52, 0xe8
};

