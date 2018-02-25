#ifndef SOURCES_OBD_OBD_H_
#define SOURCES_OBD_OBD_H_
#include "logger_frames.h"

#define OBD_PID_ERR -1

typedef struct {
	uint8_t byte_a;
	uint8_t byte_b;
	uint8_t byte_c;
	uint8_t byte_d;
} obd_pid_response_t;

typedef enum {
	obd_proto_none = 0, //used to block the acquisition task
	obd_proto_auto = 1,
	obd_proto_iso9141 = 2,
	obd_proto_kwp2000_slow = 3,
	obd_proto_kwp2000_fast = 4,
	obd_proto_can_11b_500kbps = 5,
	obd_proto_can_11b_250kbps = 6,
	obd_proto_can_29b_500kbps = 7,
	obd_proto_can_29b_250kbps = 8,
} obd_protocol_t;

void obd_init(obd_protocol_t first_protocol_to_try);
void obd_deinit_from_ISR(void);
void obd_task(void);
int32_t obd_get_pid(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response);

#endif /* SOURCES_OBD_OBD_H_ */
