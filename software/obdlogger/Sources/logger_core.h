#ifndef SOURCES_LOGGER_CORE_H_
#define SOURCES_LOGGER_CORE_H_

#include <FatFS/ff.h>
#include "logger_frames.h"
#include <obd/obd.h>
#include <stdint.h>

//Functions that can be safely called from any task
void log_pid(pid_mode_t mode, uint8_t pid, uint8_t a, uint8_t b, uint8_t c, uint8_t d);
void log_gps(void);
void log_acceleration(void);
void log_internal_diagnostics(void);
void log_battery_voltage(void);
void log_detected_protocol(obd_protocol_t protocol);


//Functions to be called only from a single task
void log_init(FIL *file_handle_ptr);
uint32_t log_task(void); //returns the number of frames saved
void log_flush(void);

#endif /* SOURCES_LOGGER_CORE_H_ */
