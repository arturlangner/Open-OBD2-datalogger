#ifndef SOURCES_OBD_OBD_PIDS_H_
#define SOURCES_OBD_OBD_PIDS_H_
#include "../logger_frames.h"
#include <stdint.h>

#define DONT_SAMPLE UINT8_MAX
#define SAMPLE_ONCE 0

uint8_t obd_pid_get_length(pid_mode_t mode, uint8_t pid);

uint8_t obd_pid_get_default_sampling_interval_seconds(pid_mode_t mode, uint8_t pid);

#endif /* SOURCES_OBD_OBD_PIDS_H_ */
