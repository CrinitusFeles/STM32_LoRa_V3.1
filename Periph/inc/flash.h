#ifndef FLASH_INC_H
#define FLASH_INC_H

#include "stm32l4xx.h"

uint8_t FLASH_mass_erase();
uint8_t FLASH_erase_page(uint8_t page_num);
uint8_t FLASH_write(uint8_t page_num, uint16_t offset, uint64_t *data, uint16_t length);
void FLASH_read(uint8_t page_num, uint16_t offset, uint64_t *buffer, uint16_t length);

#endif