#ifndef COMMON_CRC_H_
#define COMMON_CRC_H_

#include <stdint.h>

void crc_start(void);
uint32_t crc_continue(const uint32_t *data, uint32_t length_words);

void crc_deinit(void);

#endif /* COMMON_CRC_H_ */
