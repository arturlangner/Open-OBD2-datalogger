#ifndef SOURCES_OBD_OBD_CAN_H_
#define SOURCES_OBD_OBD_CAN_H_
#include "logger_frames.h"
#include "obd.h"
#include <stdbool.h>

typedef enum {
	can_speed_250kbaud,
	can_speed_500kbaud,
} can_speed_t;

#define CAN_STANDARD_ID false
#define CAN_EXTENDED_ID true

void obd_can_init(can_speed_t speed, bool use_extended_id);
void obd_can_deinit(void);
void obd_can_task(void);
int32_t obd_can_get_pid(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response);

#endif /* SOURCES_OBD_OBD_CAN_H_ */
