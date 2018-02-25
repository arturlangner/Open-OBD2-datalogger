#ifndef SOURCES_LOGGER_FRAMES_H_
#define SOURCES_LOGGER_FRAMES_H_

#include <FreeRTOS/include/FreeRTOS.h>
#include <FreeRTOS/include/task.h>
#include "minmea.h"
#include <stdint.h>

typedef enum {
	logger_frame_disabled = 0,
	logger_frame_pid = 1,
	logger_frame_gps = 2,
	logger_frame_acceleration = 3,
	logger_frame_internal_diagnostics = 4,
	logger_frame_save_used_protocol = 5,
	logger_frame_battery_voltage = 6,
} logger_frame_type_t;

typedef enum {
	pid_mode_01 = 1,
	pid_mode_02 = 2,
	pid_mode_03 = 3,
	pid_mode_04 = 4,
	pid_mode_05 = 5,
	pid_mode_09 = 9,
} pid_mode_t;

typedef struct {
	TickType_t timestamp;
	logger_frame_type_t frame_type; //always logger_frame_pid
	pid_mode_t mode;
	uint8_t pid;
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint8_t reserved1;
} frame_pid_t;

typedef struct {
	TickType_t timestamp;
	const logger_frame_type_t frame_type; //always logger_frame_internal_diagnostics
	uint8_t pid_queue_blocks;
	uint16_t acquisition_task_stack_free_minimum;
	uint16_t logging_task_stack_free_minimum;
	const uint16_t timebase_hz;
	uint8_t pid_get_failures;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t reserved3;
} frame_diagnostics_t;

typedef struct { //optimally packed :)
	TickType_t timestamp;
	const logger_frame_type_t frame_type; //always logger_frame_gps

	struct minmea_date date;
	struct minmea_time time;

	minmea_float_t longitude;
	minmea_float_t latitude;
	minmea_float_t azimuth;
	minmea_float_t speed_kph;

	bool valid;

	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t reserved3;
} frame_gps_t;

typedef struct {
	TickType_t timestamp;
	logger_frame_type_t frame_type; //always logger_frame_battery_voltage
	uint8_t power_failure_flag;
	uint16_t battery_voltage_adc_code;
} frame_battery_voltage_t;

//https://github.com/stanleyhuangyc/ArduinoOBD/blob/master/libraries/OBD/OBD.h
#define PID_ENGINE_LOAD 0x04
#define PID_COOLANT_TEMP 0x05
#define PID_SHORT_TERM_FUEL_TRIM_1 0x06
#define PID_LONG_TERM_FUEL_TRIM_1 0x07
#define PID_SHORT_TERM_FUEL_TRIM_2 0x08
#define PID_LONG_TERM_FUEL_TRIM_2 0x09
#define PID_FUEL_PRESSURE 0x0A
#define PID_INTAKE_MAP 0x0B
#define PID_RPM 0x0C
#define PID_SPEED 0x0D
#define PID_TIMING_ADVANCE 0x0E
#define PID_INTAKE_TEMP 0x0F
#define PID_MAF_FLOW 0x10
#define PID_THROTTLE 0x11
#define PID_AUX_INPUT 0x1E
#define PID_RUNTIME 0x1F
#define PID_DISTANCE_WITH_MIL 0x21
#define PID_COMMANDED_EGR 0x2C
#define PID_EGR_ERROR 0x2D
#define PID_COMMANDED_EVAPORATIVE_PURGE 0x2E
#define PID_FUEL_LEVEL 0x2F
#define PID_WARMS_UPS 0x30
#define PID_DISTANCE 0x31
#define PID_EVAP_SYS_VAPOR_PRESSURE 0x32
#define PID_BAROMETRIC 0x33
#define PID_CATALYST_TEMP_B1S1 0x3C
#define PID_CATALYST_TEMP_B2S1 0x3D
#define PID_CATALYST_TEMP_B1S2 0x3E
#define PID_CATALYST_TEMP_B2S2 0x3F
#define PID_CONTROL_MODULE_VOLTAGE 0x42
#define PID_ABSOLUTE_ENGINE_LOAD 0x43
#define PID_AIR_FUEL_EQUIV_RATIO 0x44
#define PID_RELATIVE_THROTTLE_POS 0x45
#define PID_AMBIENT_TEMP 0x46
#define PID_ABSOLUTE_THROTTLE_POS_B 0x47
#define PID_ABSOLUTE_THROTTLE_POS_C 0x48
#define PID_ACC_PEDAL_POS_D 0x49
#define PID_ACC_PEDAL_POS_E 0x4A
#define PID_ACC_PEDAL_POS_F 0x4B
#define PID_COMMANDED_THROTTLE_ACTUATOR 0x4C
#define PID_TIME_WITH_MIL 0x4D
#define PID_TIME_SINCE_CODES_CLEARED 0x4E
#define PID_ETHANOL_FUEL 0x52
#define PID_FUEL_RAIL_PRESSURE 0x59
#define PID_HYBRID_BATTERY_PERCENTAGE 0x5B
#define PID_ENGINE_OIL_TEMP 0x5C
#define PID_FUEL_INJECTION_TIMING 0x5D
#define PID_ENGINE_FUEL_RATE 0x5E
#define PID_ENGINE_TORQUE_DEMANDED 0x61
#define PID_ENGINE_TORQUE_PERCENTAGE 0x62
#define PID_ENGINE_REF_TORQUE 0x63

#endif /* SOURCES_LOGGER_FRAMES_H_ */
