#include "ff.h"

#if BOOTLOADER_BUILD == 1
//bootloader does not have GPS support, so it does not support any kind of RTC
DWORD get_fattime (void){
	return 0;
}
#else

#include <gps_core.h>

DWORD get_fattime (void){
	/* Return Value
	 * Current local time shall be returned as bit-fields packed into a DWORD value. The bit fields are as follows:
	 * bit31:25 Year origin from the 1980 (0..127, e.g. 37 for 2017)
	 * bit24:21 Month (1..12)
	 * bit20:16 Day of the month(1..31)
	 * bit15:11 Hour (0..23)
	 * bit10:5  Minute (0..59)
	 * bit4:0   Second / 2 (0..29, e.g. 25 for 50)
	 */
	DWORD timestamp = GLOBAL_frame_gps_current.time.seconds/2;
	timestamp |= GLOBAL_frame_gps_current.time.minutes << 5;
	timestamp |= GLOBAL_frame_gps_current.time.hours << 11;
	timestamp |= GLOBAL_frame_gps_current.date.day << 16;
	timestamp |= GLOBAL_frame_gps_current.date.month << 21;

	//GPS year is just two digits, eg. 2017 = 17
	timestamp |= (GLOBAL_frame_gps_current.date.year+20) << 25;

	return timestamp;
}
#endif
