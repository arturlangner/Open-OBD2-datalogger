#ifndef GPS_CORE_H_
#define GPS_CORE_H_
#include "logger_frames.h"
#include <stdbool.h>

extern frame_gps_t GLOBAL_frame_gps_current;

bool gps_core_consume_nmea(const char *line);
void gps_dump_state(void);

#endif
