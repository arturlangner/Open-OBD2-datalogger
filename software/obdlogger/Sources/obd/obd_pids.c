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
#include "obd_pids.h"

//common PID lengths taken from https://en.wikipedia.org/wiki/OBD-II_PIDs
//used mostly in ISO 9191 mode as the responses don't contain length bytes
static const uint8_t PID_INFO[256][2] = {
		//first column - length
		//second column - default sampling interval in seconds (0 - sample once, UINT8_MAX - don't sample)
		[0x0]  = { 4 , SAMPLE_ONCE }, //PIDs supported [01 - 20]
		[0x1]  = { 4 , SAMPLE_ONCE }, //Monitor status since DTCs cleared. (Includes malfunction indicator lamp (MIL) status and number of DTCs.)
		[0x2]  = { 2 , SAMPLE_ONCE }, //Freeze DTC
		[0x3]  = { 2 , 30 }, //Fuel system status
		[0x4]  = { 1 , 3 }, //Calculated engine load
		[0x5]  = { 1 , 30 }, //Engine coolant temperature
		[0x6]  = { 1 , 5 }, //Short term fuel trim—Bank 1
		[0x7]  = { 1 , 5 }, //Long term fuel trim—Bank 1
		[0x8]  = { 1 , 5 }, //Short term fuel trim—Bank 2
		[0x9]  = { 1 , 5 }, //Long term fuel trim—Bank 2
		[0x0A] = { 1 , 4 }, //Fuel pressure (gauge pressure)
		[0x0B] = { 1 , 4 }, //Intake manifold absolute pressure
		[0x0C] = { 2 , 1 }, //Engine RPM
		[0x0D] = { 1 , 2 }, //Vehicle speed
		[0x0E] = { 1 , 1 }, //Timing advance
		[0x0F] = { 1 , 10 }, //Intake air temperature
		[0x10] = { 2 , 1 }, //MAF air flow rate
		[0x11] = { 1 , 1 }, //Throttle position
		[0x12] = { 1 , 10 }, //Commanded secondary air status
		[0x13] = { 1 , 20 }, //Oxygen sensors present (in 2 banks)
		[0x14] = { 2 , 20 }, //"Oxygen Sensor 1 A: Voltage B: Short term fuel trim"
		[0x15] = { 2 , 20 }, //"Oxygen Sensor 2 A: Voltage B: Short term fuel trim"
		[0x16] = { 2 , 20 }, //"Oxygen Sensor 3 A: Voltage B: Short term fuel trim"
		[0x17] = { 2 , 20 }, //"Oxygen Sensor 4 A: Voltage B: Short term fuel trim"
		[0x18] = { 2 , 20 }, //"Oxygen Sensor 5 A: Voltage B: Short term fuel trim"
		[0x19] = { 2 , 20 }, //"Oxygen Sensor 6 A: Voltage B: Short term fuel trim"
		[0x1A] = { 2 , 20 }, //"Oxygen Sensor 7 A: Voltage B: Short term fuel trim"
		[0x1B] = { 2 , 20 }, //"Oxygen Sensor 8 A: Voltage B: Short term fuel trim"
		[0x1C] = { 1 , SAMPLE_ONCE }, //OBD standards this vehicle conforms to
		[0x1D] = { 1 , SAMPLE_ONCE }, //Oxygen sensors present (in 4 banks)
		[0x1E] = { 1 , SAMPLE_ONCE }, //Auxiliary input status
		[0x1F] = { 2 , 30 }, //Run time since engine start
		[0x20] = { 4 , SAMPLE_ONCE }, //PIDs supported [21 - 40]
		[0x21] = { 2 , SAMPLE_ONCE }, //Distance traveled with malfunction indicator lamp (MIL) on
		[0x22] = { 2 , 5 }, //Fuel Rail Pressure (relative to manifold vacuum)
		[0x23] = { 2 , 5 }, //Fuel Rail Gauge Pressure (diesel, or gasoline direct injection)
		[0x24] = { 4 , 20 }, //"Oxygen Sensor 1 AB: Fuel–Air Equivalence Ratio CD: Voltage"
		[0x25] = { 4 , 20 }, //"Oxygen Sensor 2 AB: Fuel–Air Equivalence Ratio CD: Voltage"
		[0x26] = { 4 , 20 }, //"Oxygen Sensor 3 AB: Fuel–Air Equivalence Ratio CD: Voltage"
		[0x27] = { 4 , 20 }, //"Oxygen Sensor 4 AB: Fuel–Air Equivalence Ratio CD: Voltage"
		[0x28] = { 4 , 20 }, //"Oxygen Sensor 5 AB: Fuel–Air Equivalence Ratio CD: Voltage"
		[0x29] = { 4 , 20 }, //"Oxygen Sensor 6 AB: Fuel–Air Equivalence Ratio CD: Voltage"
		[0x2A] = { 4 , 20 }, //"Oxygen Sensor 7 AB: Fuel–Air Equivalence Ratio CD: Voltage"
		[0x2B] = { 4 , 20 }, //"Oxygen Sensor 8 AB: Fuel–Air Equivalence Ratio CD: Voltage"
		[0x2C] = { 1 , 10 }, //Commanded EGR
		[0x2D] = { 1 , 10 }, //EGR Error
		[0x2E] = { 1 , 20 }, //Commanded evaporative purge
		[0x2F] = { 1 , 30 }, //Fuel Tank Level Input
		[0x30] = { 1 , 100 }, //Warm-ups since codes cleared
		[0x31] = { 2 , 100 }, //Distance traveled since codes cleared
		[0x32] = { 2 , 60 }, //Evap. System Vapor Pressure
		[0x33] = { 1 , 30 }, //Absolute Barometric Pressure
		[0x34] = { 4 , 20 }, //"Oxygen Sensor 1 AB: Fuel–Air Equivalence Ratio CD: Current"
		[0x35] = { 4 , 20 }, //"Oxygen Sensor 2 AB: Fuel–Air Equivalence Ratio CD: Current"
		[0x36] = { 4 , 20 }, //"Oxygen Sensor 3 AB: Fuel–Air Equivalence Ratio CD: Current"
		[0x37] = { 4 , 20 }, //"Oxygen Sensor 4 AB: Fuel–Air Equivalence Ratio CD: Current"
		[0x38] = { 4 , 20 }, //"Oxygen Sensor 5 AB: Fuel–Air Equivalence Ratio CD: Current"
		[0x39] = { 4 , 20 }, //"Oxygen Sensor 6 AB: Fuel–Air Equivalence Ratio CD: Current"
		[0x3A] = { 4 , 20 }, //"Oxygen Sensor 7 AB: Fuel–Air Equivalence Ratio CD: Current"
		[0x3B] = { 4 , 20 }, //"Oxygen Sensor 8 AB: Fuel–Air Equivalence Ratio CD: Current"
		[0x3C] = { 2 , 20 }, //Catalyst Temperature: Bank 1, Sensor 1
		[0x3D] = { 2 , 20 }, //Catalyst Temperature: Bank 2, Sensor 1
		[0x3E] = { 2 , 20 }, //Catalyst Temperature: Bank 1, Sensor 2
		[0x3F] = { 2 , 20 }, //Catalyst Temperature: Bank 2, Sensor 2
		[0x40] = { 4 , SAMPLE_ONCE }, //PIDs supported [41 - 60]
		[0x41] = { 4 , SAMPLE_ONCE }, //Monitor status this drive cycle
		[0x42] = { 2 , 10 }, //Control module voltage
		[0x43] = { 2 , 10 }, //Absolute load value
		[0x44] = { 2 , 10 }, //Fuel–Air commanded equivalence ratio
		[0x45] = { 1 , 10 }, //Relative throttle position
		[0x46] = { 1 , 30 }, //Ambient air temperature
		[0x47] = { 1 , 3 }, //Absolute throttle position B
		[0x48] = { 1 , 3 }, //Absolute throttle position C
		[0x49] = { 1 , 3 }, //Accelerator pedal position D
		[0x4A] = { 1 , 3 }, //Accelerator pedal position E
		[0x4B] = { 1 , 3 }, //Accelerator pedal position F
		[0x4C] = { 1 , 3 }, //Commanded throttle actuator
		[0x4D] = { 2 , 100 }, //Time run with MIL on
		[0x4E] = { 2 , 100 }, //Time since trouble codes cleared
		[0x4F] = { 4 , SAMPLE_ONCE }, //Maximum value for Fuel–Air equivalence ratio, oxygen sensor voltage, oxygen sensor current, and intake manifold absolute pressure
		[0x50] = { 4 , SAMPLE_ONCE }, //Maximum value for air flow rate from mass air flow sensor
		[0x51] = { 1 , SAMPLE_ONCE }, //Fuel Type
		[0x52] = { 1 , SAMPLE_ONCE }, //Ethanol fuel %
		[0x53] = { 2 , 30 }, //Absolute Evap system Vapor Pressure
		[0x54] = { 2 , 30 }, //Evap system vapor pressure
		[0x55] = { 2 , 20 }, //Short term secondary oxygen sensor trim, A: bank 1, B: bank 3
		[0x56] = { 2 , 20 }, //Long term secondary oxygen sensor trim, A: bank 1, B: bank 3
		[0x57] = { 2 , 20 }, //Short term secondary oxygen sensor trim, A: bank 2, B: bank 4
		[0x58] = { 2 , 20 }, //Long term secondary oxygen sensor trim, A: bank 2, B: bank 4
		[0x59] = { 2 , 5 }, //Fuel rail absolute pressure
		[0x5A] = { 1 , 5 }, //Relative accelerator pedal position
		[0x5B] = { 1 , 254 }, //Hybrid battery pack remaining life
		[0x5C] = { 1 , 10 }, //Engine oil temperature
		[0x5D] = { 2 , 5 }, //Fuel injection timing
		[0x5E] = { 2 , 2 }, //Engine fuel rate
		[0x5F] = { 1 , SAMPLE_ONCE }, //Emission requirements to which vehicle is designed
		[0x60] = { 4 , SAMPLE_ONCE }, //PIDs supported [61 - 80]
		[0x61] = { 1 , 2 }, //Driver's demand engine - percent torque
		[0x62] = { 1 , 2 }, //Actual engine - percent torque
		[0x63] = { 2 , SAMPLE_ONCE }, //Engine reference torque
		[0x64] = { 5 , DONT_SAMPLE }, //Engine percent torque data
		[0x65] = { 2 , SAMPLE_ONCE }, //Auxiliary input / output supported
		[0x66] = { 5 , SAMPLE_ONCE }, //Mass air flow sensor
		[0x67] = { 3 , 20 }, //Engine coolant temperature
		[0x68] = { 7 , DONT_SAMPLE }, //Intake air temperature sensor
		[0x69] = { 7 , DONT_SAMPLE }, //Commanded EGR and EGR Error
		[0x6A] = { 5 , DONT_SAMPLE }, //Commanded Diesel intake air flow control and relative intake air flow position
		[0x6B] = { 5 , DONT_SAMPLE }, //Exhaust gas recirculation temperature
		[0x6C] = { 5 , DONT_SAMPLE }, //Commanded throttle actuator control and relative throttle position
		[0x6D] = { 6 , DONT_SAMPLE }, //Fuel pressure control system
		[0x6E] = { 5 , DONT_SAMPLE }, //Injection pressure control system
		[0x6F] = { 3 , 3 }, //Turbocharger compressor inlet pressure
		[0x70] = { 9 , DONT_SAMPLE }, //Boost pressure control
		[0x71] = { 5 , DONT_SAMPLE }, //Variable Geometry turbo (VGT) control
		[0x72] = { 5 , DONT_SAMPLE }, //Wastegate control
		[0x73] = { 5 , DONT_SAMPLE }, //Exhaust pressure
		[0x74] = { 5 , DONT_SAMPLE }, //Turbocharger RPM
		[0x75] = { 7 , DONT_SAMPLE }, //Turbocharger temperature
		[0x76] = { 7 , DONT_SAMPLE }, //Turbocharger temperature
		[0x77] = { 5 , DONT_SAMPLE }, //Charge air cooler temperature (CACT)
		[0x78] = { 9 , DONT_SAMPLE }, //Exhaust Gas temperature (EGT) Bank 1
		[0x79] = { 9 , DONT_SAMPLE }, //Exhaust Gas temperature (EGT) Bank 2
		[0x7A] = { 7 , DONT_SAMPLE }, //Diesel particulate filter (DPF)
		[0x7B] = { 7 , DONT_SAMPLE }, //Diesel particulate filter (DPF)
		[0x7C] = { 9 , DONT_SAMPLE }, //Diesel Particulate filter (DPF) temperature
		[0x7D] = { 1 , SAMPLE_ONCE }, //NOx NTE (Not-To-Exceed) control area status
		[0x7E] = { 1 , SAMPLE_ONCE }, //PM NTE (Not-To-Exceed) control area status
		[0x7F] = { 13, DONT_SAMPLE }, //Engine run time
		[0x80] = { 4 , DONT_SAMPLE }, //PIDs supported [81 - A0]
		[0x81] = { 21, DONT_SAMPLE }, //Engine run time for Auxiliary Emissions Control Device(AECD)
		[0x82] = { 21, DONT_SAMPLE }, //Engine run time for Auxiliary Emissions Control Device(AECD)
		[0x83] = { 5 , DONT_SAMPLE }, //NOx sensor
		[0x84] = { 0/*unknown*/ , SAMPLE_ONCE }, //Manifold surface temperature
		[0x85] = { 0/*unknown*/ , SAMPLE_ONCE }, //NOx reagent system
		[0x86] = { 0/*unknown*/ , SAMPLE_ONCE }, //Particulate matter (PM) sensor
		[0x87] = { 0/*unknown*/ , SAMPLE_ONCE }, //Intake manifold absolute pressure
		[0xA0] = { 4 , SAMPLE_ONCE }, //PIDs supported [A1 - C0]
		[0xC0] = { 4 , SAMPLE_ONCE }, //PIDs supported [C1 - E0]
};

uint8_t obd_pid_get_length(pid_mode_t mode, uint8_t pid){
	if (mode == pid_mode_01 || mode == pid_mode_02){
		return PID_INFO[pid][0];
	}
	return 0; //unknown length - unsupported PID
}

uint8_t obd_pid_get_default_sampling_interval_seconds(pid_mode_t mode, uint8_t pid){
	if (mode == pid_mode_01){
		if (obd_pid_get_length(mode, pid)){ //the PID must be defined in the array above
			return PID_INFO[pid][1];
		}
	}
	return DONT_SAMPLE; //unknown PID type
}
