#pragma once
#include <stdint.h>

/* prog_info_t is placed at a fixed point in the final binary, right after interrupt vectors.
 * It is inspected by bootloader at startup to check if image is correct.
 * The binary image is appended with CRC-32.
 */

typedef enum {
	program_type_bootloader = 0,
	program_type_application = 1
} program_type_t;

typedef struct {
	uint32_t length;
	uint16_t version_major;
	uint16_t version_minor;
	program_type_t program_type;
	uint8_t reserved[23];
} prog_info_t;

_Static_assert(sizeof(prog_info_t)==32, "Wrong size?");

typedef struct {
	uint16_t version_major;
	uint16_t version_minor;
	program_type_t program_type;
	uint8_t reserved[23];
} prog_info_no_length_t;
_Static_assert(sizeof(prog_info_no_length_t)==32-sizeof(uint32_t), "Wrong size?");

extern const prog_info_t prog_info; //this symbol is provided by the linker

#define PROG_INFO_OFFSET 0x410

#define APPLICATION_BASE 20480 //first 20KB of flash holds the bootloader
