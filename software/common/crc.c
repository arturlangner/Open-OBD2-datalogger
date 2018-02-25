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
#include <crc.h>
#include <MKE06Z4.h>

void crc_start(void) { //set up hardware CRC generator
        SIM->SCGC |= SIM_SCGC_CRC_MASK; //enable clock to hardware CRC generator
        CRC0->CTRL = CRC_CTRL_WAS_MASK | CRC_CTRL_TCRC_MASK | CRC_CTRL_TOTR(2) | CRC_CTRL_TOT(2);
        CRC0->DATA = 0xffffffffu;
        CRC0->GPOLY = 0x04C11DB7;                                                         // Standard CRC-32 poly
        CRC0->CTRL = CRC_CTRL_TCRC_MASK | CRC_CTRL_TOTR(2) | CRC_CTRL_TOT(2) | CRC_CTRL_FXOR_MASK;
}

uint32_t crc_continue(const uint32_t *data, uint32_t length_words) {
        const uint32_t *p = data;
        for (uint32_t i = 0; i < length_words; i++) {
                CRC0->DATA = *p++;
        }
        return CRC0->DATA;
}

void crc_deinit(void){
	SIM->SCGC &= ~SIM_SCGC_CRC_MASK; //disable clock to hardware CRC generator
}
