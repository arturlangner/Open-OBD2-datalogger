#ifndef SOURCES_POWER_H_
#define SOURCES_POWER_H_
#include <stdbool.h>
#include <stdint.h>

/* This module monitors supply voltage and executes shutdown when car is turned off.
 *
 * This module has a common header but different implementations for bootloader and main application.
 *
 * Bootloader implementation uses ADC for power failure detection
 * and analog comparator for power-on detection.
 *
 * Application implementation functions are called only from the storage task.
 * It uses analog comparator interrupt for sleep/wakeup and ADC readings
 * (to avoid spurious shutdown due to supply noise).
 */

void power_init(void);
bool power_is_good(void); //processes data from ADC
__attribute__((noreturn)) void power_shutdown(void); //shuts down all possible components and puts the MCU into stop mode

extern volatile bool GLOBAL_power_failure_flag;

/* Input voltage divider has factor 0,1388 (eg. 14V supply -> 1,94V at MCU pin).
 * Comparator has 0-64 DAC using 3,3V as reference.
 *
 * Power failure is detected when supply falls below POWER_FAILURE_VOLTAGE.
 */
#define POWER_FAILURE_VOLTAGE 13.3f //in Volts

#define REFERENCE_VOLTAGE 3.3f
#define VOLTAGE_SCALING_FACTOR 0.1388f //resistive divider factor

#define COMPARATOR_DAC_RESOLUTION 64.0f //2^6
#define ADC_RESOLUTION 4096.0f //2^12

#define POWER_FAILURE_COMPARATOR_LEVEL ((uint8_t)(((POWER_FAILURE_VOLTAGE*VOLTAGE_SCALING_FACTOR)/REFERENCE_VOLTAGE)*COMPARATOR_DAC_RESOLUTION))
#define POWER_FAILURE_ADC_LEVEL ((uint16_t)(((POWER_FAILURE_VOLTAGE*VOLTAGE_SCALING_FACTOR)/REFERENCE_VOLTAGE)*ADC_RESOLUTION))

#define POWER_SHUTDOWN_VOLTAGE_SAMPLE_BELOW_THRESHOLD_COUNT 10
#define POWER_SHUTDOWN_VOLTAGE_SAMPLE_INTERVAL_ms 1000

_Static_assert(POWER_FAILURE_COMPARATOR_LEVEL < COMPARATOR_DAC_RESOLUTION, "Threshold voltage too high for comparator?!");
_Static_assert(POWER_FAILURE_ADC_LEVEL < ADC_RESOLUTION, "Threshold voltage too high for ADC?!");

#endif /* SOURCES_POWER_H_ */
