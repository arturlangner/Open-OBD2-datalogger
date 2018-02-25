#ifndef SOURCES_OBD_OBD_K_LINE_H_
#define SOURCES_OBD_OBD_K_LINE_H_
#include "logger_frames.h"
#include "obd.h"
#include <stdbool.h>
#include <stdint.h>

obd_protocol_t obd_k_line_init(obd_protocol_t first_protocol_to_try); //returns detected protocol or obd_proto_none
void obd_k_line_deinit_from_ISR(void);
void obd_k_line_task(void);
int32_t obd_k_line_get_pid(pid_mode_t mode, uint8_t pid, obd_pid_response_t *target_response);

#endif /* SOURCES_OBD_OBD_K_LINE_H_ */
