/* This file should be included at the beginning of every module,
 * before #include, the DEBUG_MODULE symbol should be #defined
 */

#if defined(DEBUG_TERMINAL) && DEBUG_RTT_ENABLE != 0
#if BOOTLOADER_BUILD == 0
	#include <FreeRTOS/include/FreeRTOS.h>
	#include <FreeRTOS/include/semphr.h>
	extern SemaphoreHandle_t RTT_mutex_handle;
#endif

    #include <inttypes.h>
    #include <stdio.h>
	#include "SEGGER/SEGGER_RTT.h"
#if BOOTLOADER_BUILD == 0
    #define debugf(M, ...) do { \
			xSemaphoreTake(RTT_mutex_handle, portMAX_DELAY);\
			SEGGER_RTT_printf(DEBUG_TERMINAL, "%05d %d:%s: " M "\r\n", xTaskGetTickCount(), __LINE__, __func__, ##__VA_ARGS__);\
			xSemaphoreGive(RTT_mutex_handle);\
			} while (0)
#else
#define debugf(M, ...) SEGGER_RTT_printf(DEBUG_TERMINAL, "%d:%s: " M "\r\n", __LINE__, __func__, ##__VA_ARGS__)
#endif
	#define debug_dump_buffer(len, buffer, msg) do { \
										    	SEGGER_RTT_printf(DEBUG_MODULE, "%d:%s: %s len %ld", __LINE__, __func__, msg, len);\
												for (uint32_t iiiii = 0; iiiii < len; iiiii++){ \
													if (iiiii % 16 == 0){ SEGGER_RTT_printf(0, "\n\r"); }\
													SEGGER_RTT_printf(0, "%02X", ((uint8_t*)buffer)[iiiii]);\
												} \
												SEGGER_RTT_printf(0, "\n\r");\
											} while (0)

#else
    #define debugf(...)
	#define debug_dump_buffer(...)
#endif
