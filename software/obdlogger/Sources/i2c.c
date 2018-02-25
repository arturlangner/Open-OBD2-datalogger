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
#include <FreeRTOS/include/task.h>
#include <core_cm0plus.h> //must be after FreeRTOS included
#include "i2c.h"

#define DEBUG_TERMINAL 0
#include "debug.h"

#define I2C I2C0

void i2c_init(void){
	portENTER_CRITICAL();
	SIM_SCGC |= SIM_SCGC_I2C0_MASK; //enable clock to I2C peripheral

	portEXIT_CRITICAL();

	I2C->C1 = I2C_C1_IICEN_MASK; //enable I2C
	I2C->F = I2C_F_MULT(2)/*multiply the divider by 4*/ | 0x3F/*divider is 63*/;
	debugf("I2C initialized");
}

static void _wait_for_transfer(void){
	//TODO: use ISRs and task notifications
	vTaskDelay(pdMS_TO_TICKS(1));
	uint32_t c = 0;
	while((I2C->S1 & (I2C_S_TCF_MASK | I2C_S_ARBL_MASK)) == 0){
		c++;
	} //busy wait
	debugf("Wait %ld loops", c);
//	while((I2C->S1 & I2C_S_TCF_MASK) ==0 && ((I2C->S1 & I2C_S_ARBL_MASK) == 0)){} //busy wait
}

bool i2c_read_register(uint8_t slave_address, uint8_t register_address, uint32_t length, uint8_t *target_buffer){
	I2C->C1 |= I2C_C1_TX_MASK; //enable transmitter

	I2C->C1 |= I2C_C1_MST_MASK; //send start condition
	while((I2C->S1 & I2C_S_BUSY_MASK) == 0 && (I2C->S1 & I2C_S_ARBL_MASK) == 0){} //busy wait until start condition takes effect

	I2C->D = slave_address << 1; //send slave address, LSB=0 -> write
	debugf("TX %02X", slave_address << 1);
	_wait_for_transfer();

	//check acknowledge bit
	bool ack = true;
	if (I2C->S1 & I2C_S_ARBL_MASK){ //arbitration lost - wiring problem?
		debugf("arbitration lost?! ************");
		I2C->S1 |= I2C_S_ARBL_MASK; //clear arbitration lost flag
		ack = false;
	}

	if (I2C->S1 & I2C_S_RXAK_MASK){ //slave did not acknowledge
		ack = false;
	}

	if (ack){
		debugf("Slave %02X ACK *****************", slave_address);

		I2C->D = register_address; //write register address
		debugf("TX %02X", register_address);
		_wait_for_transfer();

		if (I2C->S1 & I2C_S_RXAK_MASK){ //slave did not acknowledge
			debugf("NAK");
			ack = false;
			goto CLEANUP;
		}

		I2C->C1 |= I2C_C1_RSTA_MASK; //send repeated start condition
		while((I2C->S1 & I2C_S_BUSY_MASK) == 0 && (I2C->S1 & I2C_S_ARBL_MASK) == 0){} //busy wait until start condition takes effect

		I2C->D = (slave_address << 1) | 0x01; //send slave address, LSB=1 -> read
		_wait_for_transfer();

		if (I2C->S1 & I2C_S_RXAK_MASK){ //slave did not acknowledge
			debugf("NAK");
			ack = false;
			goto CLEANUP;
		}

		I2C->C1 &= ~(I2C_C1_TX_MASK | I2C_C1_TXAK_MASK ); //switch to master-receiver mode, send ACK bits

		*target_buffer = I2C->D; //trigger read, discard the first byte
		_wait_for_transfer();

		for (uint32_t i = 0; i < length - 1; i++){
			*target_buffer = I2C->D;
			debugf("RX byte %02X", *target_buffer);
			_wait_for_transfer();
			target_buffer++;
		}
		//FIXME: last byte transmission ack!
		I2C->C1 |= I2C_C1_TXAK_MASK; //don't acknowledge last byte
		*target_buffer = I2C->D;
		debugf("RX byte %02X", *target_buffer);
		_wait_for_transfer();

	} else {
		debugf("Slave %02X NAK", slave_address);
	}

	CLEANUP:

	I2C->C1 &= ~I2C_C1_MST_MASK; //send stop condition
	while((I2C->S1 & I2C_S_BUSY_MASK) && ((I2C->S1 & I2C_S_ARBL_MASK) == 0)){
		volatile uint32_t reg = I2C->S1;
		debugf("r=%X", reg);
	} //busy wait until start condition takes effect

	I2C->C1 = 0; //disable I2C
	return ack;
}


bool i2c_write_register(uint8_t slave_address, uint8_t register_address, uint32_t length, const uint8_t *target_buffer);
