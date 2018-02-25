#ifndef SOURCES_I2C_H_
#define SOURCES_I2C_H_

#include <stdbool.h>

void i2c_init(void);
bool i2c_start(uint8_t address);
void i2c_stop(void);

bool i2c_read_register(uint8_t slave_address, uint8_t register_address, uint32_t length, uint8_t *target_buffer);
bool i2c_write_register(uint8_t slave_address, uint8_t register_address, uint32_t length, const uint8_t *target_buffer);

#endif /* SOURCES_I2C_H_ */
