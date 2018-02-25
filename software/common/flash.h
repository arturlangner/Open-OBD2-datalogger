#ifndef COMMON_FLASH_H_
#define COMMON_FLASH_H_
#include <stdint.h>

void flash_init(void);
void flash_erase_sector(uint32_t address);
void flash_write(uint32_t target_address, const uint32_t *data, uint32_t length_words);

#define FLASH_SECTOR_SIZE 512

#endif /* COMMON_FLASH_H_ */
