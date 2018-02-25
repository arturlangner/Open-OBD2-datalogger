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
#include <debug.h>
#include <flash.h>
#include <MKE06Z4.h>
#include <proginfo.h>

//#define DEBUG_TERMINAL 0

#ifndef DEFAULT_BUS_CLOCK
#error "Bus clock not defined"
#endif

#define FLASH_CMD_ERASE_SECTOR 0x0A
#define FLASH_CMD_PROGRAM 0x06

#define FLASH_ERR_BASE				0x3000										/*!< FTMRE error base */
#define FLASH_ERR_SUCCESS			0													/*!< FTMRE sucess */
#define FLASH_ERR_INVALID_PARAM		(FLASH_ERR_BASE+1)		/*!<  invalid parameter error code*/
#define EEPROM_ERR_SINGLE_BIT_FAULT	(FLASH_ERR_BASE+2)	/*!<  EEPROM single bit fault error code*/
#define EEPROM_ERR_DOUBLE_BIT_FAULT	(FLASH_ERR_BASE+4)	/*!<  EEPROM double bits fault error code*/
#define FLASH_ERR_ACCESS			(FLASH_ERR_BASE+8)				/*!< flash access error code*/
#define FLASH_ERR_PROTECTION		(FLASH_ERR_BASE+0x10)		/*!<  flash protection error code*/
#define FLASH_ERR_MGSTAT0			(FLASH_ERR_BASE+0x11)			/*!<  flash verification error code*/
#define FLASH_ERR_MGSTAT1			(FLASH_ERR_BASE+0x12)			/*!<  flash non-correctable error code*/
#define FLASH_ERR_INIT_CCIF			(FLASH_ERR_BASE+0x14)		/*!<  flash driver init error with CCIF = 1*/
#define FLASH_ERR_INIT_FDIV			(FLASH_ERR_BASE+0x18)		/*!<  flash driver init error with wrong FDIV*/

static void flash_execute_cmd(void);
static uint16_t flash_write_2_words(uint32_t address, uint32_t word0, uint32_t word1);


void flash_init(void){
#if DEFAULT_BUS_CLOCK == 20000000u
	FTMRE->FCLKDIV = 0x13;
#else
#error "FCLKDIV not defined for this bus speed! Consult reference manual for proper speed. Otherwise flash will be damaged!"
#endif
}

void flash_erase_sector(uint32_t address){
	if (address < APPLICATION_BASE){
		return;
	}

	uint16_t u16Err = 0;

	// Check address to see if it is aligned to 4 bytes
	// Global address [1:0] must be 00.
	if(address & 0x03)
	{
		u16Err = 1;
		debugf("Sector address is not 4-byte aligned");
		goto end;
	}
	// Clear error flags
	FTMRE->FSTAT = 0x30;

	// Write index to specify the command code to be loaded
	FTMRE->FCCOBIX = 0x0;
	// Write command code and memory address bits[23:16]
	FTMRE->FCCOBHI = FLASH_CMD_ERASE_SECTOR;// EEPROM FLASH command
	FTMRE->FCCOBLO = address>>16;// memory address bits[23:16]
	// Write index to specify the lower byte memory address bits[15:0] to be loaded
	FTMRE->FCCOBIX = 0x1;
	// Write the lower byte memory address bits[15:0]
	FTMRE->FCCOBLO = address;
	FTMRE->FCCOBHI = address>>8;

	// Launch the command
	flash_execute_cmd();

	// Check error status
	if(FTMRE->FSTAT & FTMRE_FSTAT_ACCERR_MASK)	{
		u16Err |= FLASH_ERR_ACCESS;
		debugf("Err access");
	}
	if(FTMRE->FSTAT & FTMRE_FSTAT_FPVIOL_MASK)	{
		u16Err |= FLASH_ERR_PROTECTION;
		debugf("Err protection");
	}
//	if(FTMRE->FSTAT & FTMRE_FS FTMRE_FSTAT_MGSTAT0_MASK)
//	{
//		u16Err |= FLASH_ERR_MGSTAT0;
//	}
//	if(FTMRE->FSTAT & FTMRE_FSTAT_MGSTAT1_MASK)
//	{
//		u16Err |= FLASH_ERR_MGSTAT1;
//	}
end:
	debugf("status = %d", u16Err);
//	return (u16Err);
}

void flash_write(uint32_t target_address, const uint32_t *data, uint32_t length_words){
	for (uint32_t i = 0; i < length_words/2; i++){
		flash_write_2_words(target_address, data[0], data[1]);
		data += 2;
		target_address += 2*sizeof(uint32_t);
	}
}

static void flash_execute_cmd(void){
	MCM->PLACR |= MCM_PLACR_ESFC_MASK;          /* enable stalling flash controller when flash is busy */
	FTMRE->FSTAT = 0x80;
	while (!(FTMRE->FSTAT & FTMRE_FSTAT_CCIF_MASK)); //wait until command is done
}

static uint16_t flash_write_2_words(uint32_t address, uint32_t word0, uint32_t word1){
	uint16_t u16Err = FLASH_ERR_SUCCESS;

	// Check address to see if it is aligned to 4 bytes
	// Global address [1:0] must be 00.
	if(address & 0x03)
	{
		u16Err = FLASH_ERR_INVALID_PARAM;
		return (u16Err);
	}
	// Clear error flags
	FTMRE->FSTAT = 0x30;

	// Write index to specify the command code to be loaded
	FTMRE->FCCOBIX = 0x0;
	// Write command code and memory address bits[23:16]
	FTMRE->FCCOBHI = FLASH_CMD_PROGRAM;// program FLASH command
	FTMRE->FCCOBLO = address>>16;// memory address bits[23:16]
	// Write index to specify the lower byte memory address bits[15:0] to be loaded
	FTMRE->FCCOBIX = 0x1;
	// Write the lower byte memory address bits[15:0]
	FTMRE->FCCOBLO = address;
	FTMRE->FCCOBHI = address>>8;

	// Write index to specify the word0 (MSB word) to be programmed
	FTMRE->FCCOBIX = 0x2;
	// Write the word 0
	//FTMRE_FCCOB = (word0) & 0xFFFF;
	FTMRE->FCCOBHI = (word0) >>8;
	FTMRE->FCCOBLO = (word0);

	// Write index to specify the word1 (LSB word) to be programmed
	FTMRE->FCCOBIX = 0x3;
	// Write word 1
	FTMRE->FCCOBHI = (word0>>16)>>8;
	FTMRE->FCCOBLO = (word0>>16);

	// Write index to specify the word0 (MSB word) to be programmed
	FTMRE->FCCOBIX = 0x4;
	// Write the word2
	//FTMRE_FCCOB = (word1) & 0xFFFF;
	FTMRE->FCCOBHI = (word1) >>8;
	FTMRE->FCCOBLO = (word1);

	// Write index to specify the word1 (LSB word) to be programmed
	FTMRE->FCCOBIX = 0x5;
	// Write word 3
	//FTMRE_FCCOB = (word1>>16) & 0xFFFF;
	FTMRE->FCCOBHI = (word1>>16)>>8;
	FTMRE->FCCOBLO = (word1>>16);

	// Launch the command
	flash_execute_cmd();

	// Check error status
	if(FTMRE->FSTAT & FTMRE_FSTAT_ACCERR_MASK)
	{
		u16Err |= FLASH_ERR_ACCESS;
	}
	if(FTMRE->FSTAT & FTMRE_FSTAT_FPVIOL_MASK)
	{
		u16Err |= FLASH_ERR_PROTECTION;
	}
//	if(FTMRE->FSTAT & FTMRE_FSTAT_MGSTAT0_MASK)
//	{
//		u16Err |= FLASH_ERR_MGSTAT0;
//	}
//	if(FTMRE->FSTAT & FTMRE_FSTAT_MGSTAT1_MASK)
//	{
//		u16Err |= FLASH_ERR_MGSTAT1;
//	}

	return (u16Err);
}
