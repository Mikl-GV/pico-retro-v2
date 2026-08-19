#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdbool.h>
#include <stdint.h>

#define SD_BLOCK_SIZE 512

bool sd_init(void);
bool sd_read_block(uint32_t block, uint8_t *buf);
bool sd_write_block(uint32_t block, const uint8_t *buf);

#endif
