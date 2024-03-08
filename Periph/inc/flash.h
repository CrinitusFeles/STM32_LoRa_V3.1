#ifndef FLASH_INC_H
#define FLASH_INC_H

#include "stm32l4xx.h"

#define FLASH_APP1_START_ADDRESS ((uint32_t)0x08000000)
#define FLASH_APP2_START_ADDRESS ((uint32_t)0x08010000)
#define EMBEDDED_FLASH_ADDRESS      0x08000000
#define FLASH_APP_END_ADDRESS   ((uint32_t)FLASH_BANK1_END-0x10) /**< Leave a little extra space at the end. */

/* Status report for the functions. */
typedef enum {
  FLASH_OK                  = 0x00, /**< The action was successful. */
  FLASH_ERROR_SIZE          = 0x01, /**< The binary is too big. */
  FLASH_ERROR_WRITE         = 0x02, /**< Writing failed. */
  FLASH_ADDRESS_NOT_EMPTY   = 0x03,
  FLASH_ERROR_READBACK      = 0x04, /**< Writing was successful, but the content of the memory is wrong. */
  FLASH_LOCKED              = 0x05,
  FLASH_TIMEOUT             = 0x06,
  FLASH_INCORRECT_ADDRESS   = 0x07,
  FLASH_ERROR               = 0xFF,  /**< Generic error. */
} FLASH_status;

FLASH_status FLASH_mass_erase();
FLASH_status FLASH_erase_page(uint8_t page_num);
FLASH_status FLASH_write(uint8_t page_num, uint16_t offset, uint64_t *data, uint16_t length);
FLASH_status FLASH_write_addr(uint32_t *addr, uint64_t *data, uint16_t word_length);
void FLASH_read(uint8_t page_num, uint16_t offset, uint64_t *buffer, uint16_t length);
void FLASH_jump_to_app(uint32_t *addr);
uint8_t SetPrefferedBlockNum(uint8_t block_num);

#endif