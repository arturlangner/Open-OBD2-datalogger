#ifndef SOURCES_ADC_H_
#define SOURCES_ADC_H_
#include <stdint.h>

/* ADC operates in free-running mode. It is used for battery voltage measurement only.
 * Voltage can be read at any time.
 */

void adc_init(void);
void adc_deinit(void);
uint32_t adc_get_result(void);

#endif /* SOURCES_ADC_H_ */
