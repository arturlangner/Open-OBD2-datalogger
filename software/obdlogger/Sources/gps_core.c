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
#include <FreeRTOS/include/FreeRTOS.h>
#include "gps_core.h"
#include "minmea.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "misc.h"

#define DEBUG_ID DEBUG_ID_GPS_CORE
#include <debug.h>

/* --------- public data ---------------- */
frame_gps_t GLOBAL_frame_gps_current = { .frame_type = logger_frame_gps, .valid = false };

/* --------- private data --------------- */

/* --------- private prototypes --------- */

/* ----------- implementation ----------- */

bool gps_core_consume_nmea(const char *line){
	uint32_t timestamp = xTaskGetTickCount();

    switch (minmea_sentence_id(line, false)) {
        case MINMEA_SENTENCE_RMC: {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line)) {

            	//always accept date from GPS - its RTC should be accurate enough without a fix
            	memcpy(&GLOBAL_frame_gps_current.date, &frame.date, sizeof(GLOBAL_frame_gps_current.date));

                if (unlikely(!frame.valid)) { return false; }
                GLOBAL_frame_gps_current.valid = true;
                /* ----- copying current GPS data into state struct ----- */
                memcpy(&GLOBAL_frame_gps_current.latitude, &frame.latitude, sizeof(minmea_float_t));
                memcpy(&GLOBAL_frame_gps_current.longitude, &frame.longitude, sizeof(minmea_float_t));
                GLOBAL_frame_gps_current.timestamp = timestamp;
                /* ------------------------------------------------------ */

//                 debugf("$xxRMC: raw coordinates and speed: (%ld/%ld,%ld/%ld) %ld/%ld",
//                         frame.latitude.value, frame.latitude.scale,
//                         frame.longitude.value, frame.longitude.scale,
//                         frame.speed.value, frame.speed.scale);
//                 debugf("$xxRMC fixed-point coordinates and speed scaled to three decimal places: (%ld,%ld) %ld",
//                         minmea_rescale(&frame.latitude, 1000),
//                         minmea_rescale(&frame.longitude, 1000),
//                         minmea_rescale(&frame.speed, 1000));
//                 debugf("$xxRMC floating point degree coordinates and speed: (%f,%f) %f",
//                         minmea_tocoord(&frame.latitude),
//                         minmea_tocoord(&frame.longitude),
//                         minmea_tofloat(&frame.speed));
            } else {
                debugf("$xxRMC sentence is not parsed");
            }
        } break;

        //this sentence carries current time
        case MINMEA_SENTENCE_GGA: {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line)) {
                //debugf("$xxGGA: fix quality: %d", frame.fix_quality);
                memcpy(&GLOBAL_frame_gps_current.time, &frame.time, sizeof(GLOBAL_frame_gps_current.time));
            }
            else {
                debugf("$xxGGA sentence is not parsed");
            }
        } break;

//         case MINMEA_SENTENCE_GSV: {
//             struct minmea_sentence_gsv frame;
//             if (minmea_parse_gsv(&frame, line)) {
//                 debugf("$xxGSV: message %d of %d\n", frame.msg_nr, frame.total_msgs);
//                 debugf("$xxGSV: sattelites in view: %d\n", frame.total_sats);
//                 for (int i = 0; i < 4; i++)
//                     debugf("$xxGSV: sat nr %d, elevation: %d, azimuth: %d, snr: %d dbm\n",
//                         frame.sats[i].nr,
//                         frame.sats[i].elevation,
//                         frame.sats[i].azimuth,
//                         frame.sats[i].snr);
//             }
//             else {
//                 debugf("$xxGSV sentence is not parsed\n");
//             }
//         } break;

        case MINMEA_SENTENCE_VTG: {
            struct minmea_sentence_vtg frame;
            if (minmea_parse_vtg(&frame, line)) {
                /* ----- copying current GPS data into state struct ----- */
                memcpy(&GLOBAL_frame_gps_current.azimuth, &frame.true_track_degrees, sizeof(minmea_float_t));
                memcpy(&GLOBAL_frame_gps_current.speed_kph, &frame.speed_kph, sizeof(minmea_float_t));
                GLOBAL_frame_gps_current.timestamp = timestamp;
                /* ------------------------------------------------------ */

//                 debugf("$xxVTG: true track degrees = %f",
//                         minmea_tofloat(&frame.true_track_degrees));
//                 debugf("        magnetic track degrees = %f",
//                         minmea_tofloat(&frame.magnetic_track_degrees));
//                 debugf("        speed knots = %f",
//                         minmea_tofloat(&frame.speed_knots));
//                 debugf("        speed kph = %f",
//                         minmea_tofloat(&frame.speed_kph));
            }
            else {
                debugf("$xxVTG sentence is not parsed");
            }
        } break;

        case MINMEA_INVALID: {
            debugf("sentence is not valid <%s>", line);
            return false;
        } break;

        default: {
            //debugf("%.8s sentence is not parsed", line);
        } break;
    }
    return true;
}

void gps_dump_state(void){
    debugf("Lat %ld %ld", GLOBAL_frame_gps_current.latitude.value, GLOBAL_frame_gps_current.latitude.scale);
    debugf("Lon %ld %ld", GLOBAL_frame_gps_current.longitude.value, GLOBAL_frame_gps_current.longitude.scale);
    debugf("Az  %ld %ld", GLOBAL_frame_gps_current.azimuth.value, GLOBAL_frame_gps_current.azimuth.scale);
    debugf("Speed  %ld %ld", GLOBAL_frame_gps_current.speed_kph.value, GLOBAL_frame_gps_current.speed_kph.scale);
    debugf("Date %d:%d:%d Time %d:%d:%d", GLOBAL_frame_gps_current.date.year,
         GLOBAL_frame_gps_current.date.month, GLOBAL_frame_gps_current.date.day,
         GLOBAL_frame_gps_current.time.hours, GLOBAL_frame_gps_current.time.minutes,
         GLOBAL_frame_gps_current.time.seconds);
    debugf("timestamp %ld", GLOBAL_frame_gps_current.timestamp);
    debugf("valid = %d", GLOBAL_frame_gps_current.valid);
}
