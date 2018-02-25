#pragma once

#include <stdarg.h>
#include <stdbool.h>

typedef enum {
    DEBUG_ID_DISABLED = 0,
    DEBUG_ID_MAIN = 1,
	DEBUG_ID_SPI = 2,

	DEBUG_ID_OBD = 3,
	DEBUG_ID_OBD_CAN = 4,
	DEBUG_ID_OBD_K_LINE = 5,
	DEBUG_ID_OBD_UART = 6,

	DEBUG_ID_ACQUISITION_TASK = 7,
	DEBUG_ID_STORAGE_TASK = 8,
	DEBUG_ID_LOGGER_CORE = 9,

    DEBUG_ID_GPS_UART = 10,
    DEBUG_ID_GPS_CORE = 11,

	DEBUG_ID_CONSOLE = 12,
	DEBUG_ID_DEBUG = 13,

	DEBUG_ID_COUNT,
} debug_id_t;

#define ENABLE_RTT_OUTPUT 0x80 //this flag is combined with debug_id_t

#ifdef DEBUG_ID

// ! ! ! CAN NOT BE CALLED FROM INTERRUPT ! ! !
#define debugf(format, ...) debug_printf(DEBUG_ID, " %d:%s " format "\n", __LINE__, __func__, ##__VA_ARGS__)
// ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !

#else
#define debugf(...)
#endif

#define debug_dump_buffer(...) //TODO

void debug_init(void); //call very early in main before any debug output
void debug_printf(debug_id_t id, const char *format, ...) __attribute__ ((format (printf, 2, 3))); //can be called from any task
void debug_enable_id(debug_id_t id, bool enable);

void debug_file_init(void); //to be called only within the FatFS parent task
void debug_file_task(void); //to be called only within the FatFS parent task
void debug_sync(void); //to be called only within the FatFS parent task
