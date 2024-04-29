#ifndef FLASH_INC_H
#define FLASH_INC_H

#include "stm32l4xx.h"
#define FLASH_PAGE_SIZE             0x800
#define FLASH_START_ADDR            0x8000000
#define FLASH_PAGE_2_POW            11
#define FLASH_MAX_SIZE              0x40000
#define FLASH_EMPTY_CEIL            0xFFFFFFFFFFFFFFFF
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
  FLASH_FULL                = 0x08,
  FLASH_ERROR               = 0xFF,  /**< Generic error. */
} FLASH_status;

FLASH_status FLASH_mass_erase();
FLASH_status FLASH_erase_page(uint8_t page_num);
FLASH_status FLASH_write_page(uint8_t page_num, uint16_t offset, uint64_t *data, uint16_t length);
FLASH_status FLASH_write(uint32_t addr, uint64_t *data, uint16_t word_length);
void FLASH_read(uint8_t page_num, uint16_t offset, uint64_t *buffer, uint16_t length);
void FLASH_jump_to_app(uint32_t addr);
FLASH_status SetPrefferedBlockNum(uint8_t block_num);
FLASH_status FLASH_erase_firmware(uint8_t current_block_num);

#endif