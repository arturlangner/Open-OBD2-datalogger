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
#include <adc.h>
#include <bitbang_uart.h>
#include <cifra/aes.h>
#include <cifra/modes.h>
#include <crash_handler.h>
#include <MKE06Z4.h>
#include <core_cm0plus.h>
#include <crc.h>
#include "delay.h"
#include <FatFS/ff.h>
#include <flash.h>
#include <keys.h>
#include <log.h>
#include <pins.h>
#include <proginfo.h>
#include <power.h>
#if DEBUG_RTT_ENABLE == 1
#include <SEGGER/SEGGER_RTT.h>
#endif
#include <stdbool.h>
#include <string.h>
#include "systick.h"
#include <timer.h>

#define DEBUG_TERMINAL 0
#include <debug.h>

static FATFS _fat;
static uint8_t _read_buffer[FLASH_SECTOR_SIZE*3];
static uint8_t _decrypted_buffer[FLASH_SECTOR_SIZE*3];

static cf_cbc cbc_context;
static cf_aes_context aes_context;
static cf_cmac cmac_context;

_Static_assert(FLASH_SECTOR_SIZE % AES_BLOCKSZ == 0, "Sector size is not multiple of AES block size?");
_Static_assert(PROG_INFO_OFFSET < sizeof(_decrypted_buffer), "Prog info section must fit somehow into decryption buffer to check version");

static void blink_of_death(uint32_t delay);
static uint8_t verify_firmware_file(FIL *file_handle);
static uint8_t install_firmware_from_file(FIL *file_handle, bool force_install);
static uint8_t verify_internal_firmware(uint16_t *version_major, uint16_t *version_minor);
static void start_application(void) __attribute__((noreturn));

int main(void){

	LED_INIT();
	LED1_ON();
	LED2_ON();
	LED3_ON();

	timer_init();
	bitbang_uart_init();

#if DEBUG_RTT_ENABLE == 1
	SEGGER_RTT_Init();
#endif
	systick_init();

	FRESULT r;
	for (uint32_t i = 0; i < 5; i++){
		r = f_mount(&_fat, "", 1/*force mount now*/);
		debugf("attempt %ld mount = %d", i, r);
		if (r == FR_OK){
			break;
		} else {
			delay_ms(200);
		}
	}

	/* Wake the GPS in case the bootloader will have to put it to sleep.
	 * At startup the bootloader does not know if the GPS is awake.
	 * By only sending the sleep command it might actually wake up
	 * the module, as it requires some time to boot and wakes on any character.
	 * Waking it up explicitly here guarantees that it will respond to sleep
	 * command sent later properly.
	 */
	static const char GPS_WAKE_STRING[] = "\r\n\r\n";
	bitbang_uart_transmit(sizeof(GPS_WAKE_STRING)-1, (uint8_t*)GPS_WAKE_STRING, NULL/*no callback*/);

	if (r){ //failed to mount SD card
		blink_of_death(500);
	}

	uint16_t current_version_major, current_version_minor;
	DIR directory_struct;
	FRESULT opendir_status = f_opendir(&directory_struct, "obdlog");
	debugf("opendir = %d", opendir_status);

	if (opendir_status == FR_OK){ //there is an "OBDLOG" directory - let's see if it has new firmware file inside

		FILINFO file_info;
		while (1){
			FRESULT readdir_result = f_readdir(&directory_struct, &file_info);
			if (readdir_result == FR_OK){
				if (strlen(file_info.fname) == 0){
					debugf("ls done");
					break;
				}
				debugf("file name: %s", file_info.fname);
				char *dot = strrchr(file_info.fname, '.'); //find rightmost dot
				if (dot && !strcmp(dot, ".BIN")){ //check if file name ends with .bin
					log_start();

					char path[24] = "obdlog/";
					strncat(path, file_info.fname, sizeof(path));

					log_format("Checking file %s", file_info.fname);

					FIL file_handle;
					FRESULT open_status =  f_open(&file_handle, path, FA_OPEN_EXISTING | FA_READ);
					debugf("open file status = %d", open_status);

					if (open_status != FR_OK){
						debugf("can't open file %s", path);
						break;
					}

					//rename the file so it will not be verified the next time
					char new_file_path[32];
					strncpy(new_file_path, path, sizeof(new_file_path));
					new_file_path[strlen(new_file_path)-1] = '_'; //replace .bin with .bi_ so next time the device will not update itself
					f_rename(path, new_file_path);
					log_format("File renamed to %s", new_file_path);

					uint8_t file_verify_status = verify_firmware_file(&file_handle);
					if (file_verify_status == 0){
						log_entry("File verified OK");
						//we have a cryptographically valid firmware image on the SD card
						bool internal_firmware_status = verify_internal_firmware(&current_version_major, &current_version_minor);
						if (internal_firmware_status == 0){
							log_format("Current version %d.%d", current_version_major, current_version_minor);
						} else {
							log_format("No valid internal firmware %d", internal_firmware_status);
						}

						uint8_t installation_status = install_firmware_from_file(&file_handle, internal_firmware_status/*force install*/);
						if (installation_status == 0){
							log_entry("New firmware installed");

							uint8_t status = verify_internal_firmware(&current_version_major, &current_version_minor);
							if (status == 0){
								log_entry("New firmware okay, starting...");
								strcpy(GLOBAL_news_from_the_grave, "!upgd_ok");
								break;

							} else {
								log_format("New firmware verification failure %d", status);
							}
						} else {
							log_format("New firmware installation failure %d", installation_status);
						}
					} else {//end if firmware file signature verification okay
						log_format("File verification failure %d", file_verify_status);
					}
					f_close(&file_handle);
				} //end if file ends with .bin

			} else {
				//no OBDLOG directory found
				break;
			}
		} //end of loop

	} //else there is no OBDLOG directory, so no new firmware

	log_close();

	if (verify_internal_firmware(&current_version_major, &current_version_minor) == 0){
		debugf("Current version %d.%d", current_version_major, current_version_minor);
		start_application();
	} else {
		blink_of_death(100);
	}

	return 0;
}

static void blink_of_death(uint32_t delay){
	power_init();
	adc_init();
	while(1){
		LED1_ON();
		LED2_ON();
		LED3_ON();
		delay_ms(delay);
		LED1_OFF();
		LED2_OFF();
		LED3_OFF();
		delay_ms(delay);
		if (power_is_good() == false){
			static const char GPS_SLEEP_STRING[] = "$PMTK161,0*28\r\n";
			bitbang_uart_transmit(sizeof(GPS_SLEEP_STRING)-1, (uint8_t*)GPS_SLEEP_STRING, NULL/*no callback*/);
			bitbang_uart_deinit(); //this will block until all data is sent
			timer_deinit_from_ISR();

			power_shutdown();
		}
	}
}

static void blink_update_pattern(void){
	static uint32_t state = 0;
	switch (state){
	default:
	case 0:
		LED1_ON();
		LED2_OFF();
		LED3_OFF();
		break;
	case 1:
		LED1_OFF();
		LED2_ON();
		LED3_OFF();
		break;
	case 2:
		LED1_OFF();
		LED2_OFF();
		LED3_ON();
		state = 0;
		return;
	}
	state++;
}


static uint8_t verify_firmware_file(FIL *file_handle){
	static uint8_t signature[AES_BLOCKSZ];

	cf_aes_init(&aes_context, authentication_key, AES_BLOCKSZ);
	cf_cmac_init(&cmac_context, &cf_aes, &aes_context);

	FSIZE_t bytes_to_read = f_size(file_handle);

	uint32_t bytes_read = 0;

	cf_cmac_stream stream;
	stream.cmac = cmac_context;
	cf_cmac_stream_reset(&stream);

	//the file has 16 byte MAC at the end, so everything has to be read and processed EXCEPT the last 16 bytes
	do {
		blink_update_pattern();
		if (f_read(file_handle, _read_buffer, AES_BLOCKSZ, &bytes_read) == FR_OK){

			//			debug_dump_buffer(sizeof(chunk), chunk, "chunk");
			cf_cmac_stream_update(&stream, _read_buffer, AES_BLOCKSZ, 0/*is not final*/);

			bytes_to_read -= bytes_read;
			if (bytes_to_read % 10 == 0){
				//				debugf("bytes left %ld", bytes_to_read);
			}
		} else {
			debugf("read failed");
			return 1;
		}
	} while (bytes_to_read > 2*AES_BLOCKSZ);

	//last payload chunk has to be passed to the crypto library with a different flag
	if (f_read(file_handle, _read_buffer, AES_BLOCKSZ, &bytes_read) == FR_OK){
		//		debug_dump_buffer(sizeof(chunk), chunk, "chunk");
		cf_cmac_stream_update(&stream, _read_buffer, AES_BLOCKSZ, 1/*finalize*/);
	} else {
		debugf("Last chunk read failure?");
		return 2;
	}

	cf_cmac_stream_final(&stream, signature);

	debugf("file read done, bytes left %ld", bytes_to_read);

	//read last 16 bytes - the MAC
	if (f_read(file_handle, _read_buffer, AES_BLOCKSZ, &bytes_read) == FR_OK){
		//now signature holds the computed signature and chunk holds the stored signature - compare them
		//		debug_dump_buffer(AES_BLOCKSZ, signature, "computed signature");
		//		debug_dump_buffer(AES_BLOCKSZ, chunk, "stored signature");
		for (uint32_t i = 0; i < AES_BLOCKSZ; i++){
			if (_read_buffer[i] != signature[i]){
				debugf("Signatures differ!");
				return 3;
			}
		}
		debugf("Signature okay");
		return 0;

	} else {
		debugf("can't read MAC?");
		return 4;
	}
}

static uint8_t install_firmware_from_file(FIL *file_handle, bool force_install){
	if (f_lseek(file_handle, 0) != FR_OK){
		debugf("Can't seek!");
		return 1;
	}
	flash_init();

	FSIZE_t bytes_to_read = f_size(file_handle) - AES_BLOCKSZ; //skip last 16 bytes - they hold the MAC, it does not go to flash
	uint32_t bytes_read = 0;

	cf_aes_init(&aes_context, encryption_key, AES_BLOCKSZ);
	cf_cbc_init(&cbc_context, &cf_aes, &aes_context, encryption_iv);

	//first step - read, decrypt and discard the nonce
	FRESULT status = 0;
	status |= f_read(file_handle, _read_buffer, AES_BLOCKSZ, &bytes_read);
	cf_cbc_decrypt(&cbc_context, _read_buffer, _decrypted_buffer, 1); //decrypt nonce (one block)
	bytes_to_read -= AES_BLOCKSZ;

	//second step - read first 3 sectors (prog info section is in the third)
	status |= f_read(file_handle, _read_buffer, FLASH_SECTOR_SIZE, &bytes_read);
	status |= f_read(file_handle, _read_buffer + FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE, &bytes_read);
	status |= f_read(file_handle, _read_buffer + 2*FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE, &bytes_read);
	bytes_to_read -= 3*FLASH_SECTOR_SIZE;

	cf_cbc_decrypt(&cbc_context, _read_buffer, _decrypted_buffer, 3*FLASH_SECTOR_SIZE/AES_BLOCKSZ); //decrypt a sector

	if (status != FR_OK){
		debugf("Can't read first three sectors");
		return 2;
	}

	prog_info_t *pi_new = (prog_info_t*)(_decrypted_buffer + PROG_INFO_OFFSET);
	if (!force_install){
		prog_info_t *pi_current = (prog_info_t*)(APPLICATION_BASE + PROG_INFO_OFFSET);
		debugf("Version installed %d.%d, file %d.%d",
				pi_current->version_major,
				pi_current->version_minor,
				pi_new->version_major,
				pi_new->version_minor);
		//		debug_dump_buffer(sizeof(prog_info_t), pi_new, "prog info new");
		if (pi_new->version_major < pi_current->version_major){
			return 3;
		}
		if (pi_new->version_major == pi_current->version_major &&
				pi_new->version_minor <= pi_current->version_minor){
			return 4;
		}
	}
	log_format("New firmware version %d.%d", pi_new->version_major, pi_new->version_minor);

	//write first three sectors
	//uint32_t sector_count = 3;
	uint32_t target_address = APPLICATION_BASE;
	flash_erase_sector(target_address);
	flash_write(target_address, (uint32_t*)_decrypted_buffer, FLASH_SECTOR_SIZE/sizeof(uint32_t));//length is in words
	target_address += FLASH_SECTOR_SIZE;
	flash_erase_sector(target_address);
	flash_write(target_address, (uint32_t*)(_decrypted_buffer+FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE/sizeof(uint32_t));//length is in words
	target_address += FLASH_SECTOR_SIZE;
	flash_erase_sector(target_address);
	flash_write(target_address, (uint32_t*)(_decrypted_buffer+2*FLASH_SECTOR_SIZE), FLASH_SECTOR_SIZE/sizeof(uint32_t));//length is in words
	target_address += FLASH_SECTOR_SIZE;

	//third step - write all other sectors
	do {
		blink_update_pattern();
		if (f_read(file_handle, _read_buffer, FLASH_SECTOR_SIZE, &bytes_read) != FR_OK){
			debugf("Can't read block");
			return 5;
		}
		if (bytes_read == 0){ //end of file
			break;
		}
		//		debugf("c = %ld target = %ld left = %ld read = %ld", sector_count++, target_address, bytes_to_read, bytes_read);
		//		debug_dump_buffer(FLASH_SECTOR_SIZE, _decrypted_buffer, "sector data");
		cf_cbc_decrypt(&cbc_context, _read_buffer, _decrypted_buffer, FLASH_SECTOR_SIZE/AES_BLOCKSZ);
		flash_erase_sector(target_address);
		flash_write(target_address, (uint32_t*)_decrypted_buffer, FLASH_SECTOR_SIZE/sizeof(uint32_t));//length is in words
		target_address += bytes_read;
		bytes_to_read -= bytes_read;
	} while (bytes_read);
	debugf("Write to flash done");
	return 0;
}

static uint8_t verify_internal_firmware(uint16_t *version_major, uint16_t *version_minor){
	prog_info_t *pi = (prog_info_t*)(APPLICATION_BASE+PROG_INFO_OFFSET);
	//	debug_dump_buffer(sizeof(prog_info_t), pi, "prog info");

	if (pi->program_type != program_type_application){
		debugf("Program type mismatch %02X", pi->program_type);
		return 1;
	}

	if (pi->length > (128*1024 - APPLICATION_BASE - sizeof(uint32_t)/*CRC32*/)){
		debugf("Wrong length %ld", pi->length);
		return 2;
	}

	uint32_t *stored_crc = (uint32_t*)(APPLICATION_BASE + pi->length);

	debugf("pt=%d v=%d.%d len=%ld crc at %p = %04X", pi->program_type, pi->version_major, pi->version_minor, pi->length, stored_crc, *stored_crc);

	crc_start();
	uint32_t computed_crc = crc_continue((uint32_t*)APPLICATION_BASE, pi->length/sizeof(uint32_t));

	if (*stored_crc != computed_crc){
		debugf("Length %ld, wrong CRC: stored %04X, computed %04X", pi->length, *stored_crc, computed_crc);
		return 3;
	}

	*version_major = pi->version_major;
	*version_minor = pi->version_minor;
	return 0;
}

__attribute__((noreturn)) static void start_application(void){
	crc_deinit();

	debugf("Starting application");
	__disable_irq();
	SCB->VTOR = APPLICATION_BASE; //move interrupt vector base
	typedef void (*function_void_t)(void);
	#define avuint32(avar) (*((volatile uint32_t *) (avar)))

	function_void_t application;
	application = (function_void_t)(avuint32(APPLICATION_BASE+4));

	__DSB();

	application();
	__builtin_unreachable();
}

#if !defined(DEBUG_RTT_ENABLE)
#error "DEBUG_RTT_ENABLE must be defined 1 or 0"
#endif
