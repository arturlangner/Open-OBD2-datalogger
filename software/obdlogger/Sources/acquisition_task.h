#ifndef SOURCES_ACQUISITION_TASK_H_
#define SOURCES_ACQUISITION_TASK_H_
#include <obd/obd.h>
#include "logger_frames.h"

typedef struct {
	logger_frame_type_t channel_type;
	pid_mode_t pid_mode;
	uint8_t pid;
	uint8_t failure_count;
	TickType_t interval;
	TickType_t next_sample_timestamp;
} acquisition_channel_t;

void acquisition_task(void *params __attribute__((unused)));

void acquisition_add_channel(const acquisition_channel_t *channel);
void acquisition_start(obd_protocol_t first_protocol_to_try, bool use_default_config);

#endif /* SOURCES_ACQUISITION_TASK_H_ */
