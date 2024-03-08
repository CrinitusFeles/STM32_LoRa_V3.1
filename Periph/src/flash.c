
#include "flash.h"
#include "system_select.h"

#define KEY1                        0x45670123
#define KEY2                        0xCDEF89AB
#define FLASH_PAGE_SIZE             0x800
#define FLASH_PAGE_2_POW            11
#define FLASH_MAX_SIZE              0x40000
#define FLASH_OPERATION_TIMEOUT     100000
#define OFFSET_ALIGN_BYTES          8 // 64 bit word
#define FLASH_PGERR       (FLASH_SR_PGSERR | FLASH_SR_PGAERR | FLASH_SR_WRPERR)

#define MOD(x, y) (x & (y - 1))  // y must be power of 2!

void FLASH_wait_operation(){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    while((FLASH->SR & FLASH_SR_BSY) && (timeout > 0 )) timeout--;
    if (FLASH->SR & FLASH_SR_EOP) {
		FLASH->SR = FLASH_SR_EOP;
	}
}

FLASH_status FLASH_UNLOCK(){
    if(FLASH->CR & FLASH_CR_LOCK){
        FLASH->KEYR = KEY1;
        FLASH->KEYR = KEY2;
        FLASH_wait_operation();
    }
    if(FLASH->CR & FLASH_CR_LOCK)
        return FLASH_LOCKED;  // FLASH is LOCKED
    return FLASH_OK;
}


/// @brief For next unlocking need to call FLASH_unlock().
void FLASH_soft_lock(){
    FLASH->CR |= FLASH_CR_LOCK;
    FLASH_wait_operation();
}

/// @brief Fully block embedded FLASH. It is possible to unlock only after reset.
void FLASH_hard_lock(){
    FLASH->CR |= FLASH_CR_LOCK;
    FLASH->KEYR = 0;
    FLASH_wait_operation();
}


FLASH_status FLASH_write(uint8_t page_num, uint16_t offset, uint64_t *data, uint16_t length){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    if(!FLASH_UNLOCK())
        return FLASH_LOCKED; // FLASH is LOCKED
    uint64_t flash_data = (*(__IO uint64_t*)(EMBEDDED_FLASH_ADDRESS + page_num * FLASH_PAGE_SIZE + offset * OFFSET_ALIGN_BYTES));
    if(flash_data != 0xFFFFFFFFFFFFFFFF){  // area is empty
        FLASH_soft_lock();
        return FLASH_ADDRESS_NOT_EMPTY;
    }

    FLASH->CR &= !(FLASH_CR_PNB | FLASH_CR_MER1 | FLASH_CR_PER | FLASH_CR_FSTPG);
    FLASH->CR |= (page_num << FLASH_CR_PNB_Pos) | FLASH_CR_PG;
    for(uint16_t i = 0; i < length; i++){
        if(FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR))
            return FLASH_ERROR_WRITE;
        (*(__IO uint64_t*)(EMBEDDED_FLASH_ADDRESS + page_num * FLASH_PAGE_SIZE + (offset + i) * OFFSET_ALIGN_BYTES)) = data[i];
        while((!(FLASH->SR & FLASH_SR_EOP)) && (timeout > 0)) timeout--;
        FLASH->SR = FLASH_SR_EOP;
    }
    FLASH_soft_lock();
    return FLASH_OK;

}

FLASH_status FLASH_write_addr(uint32_t *addr, uint64_t *data, uint16_t word_length){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    if(*addr < 0x800000 && *addr >= 0x800000 + FLASH_MAX_SIZE){
        return FLASH_INCORRECT_ADDRESS;
    }
    __disable_irq();
    if(FLASH_UNLOCK() != FLASH_OK){
         __enable_irq();
        return FLASH_LOCKED; // FLASH is LOCKED
    }
#ifndef FLASH_BIT_ACCESS
    if(*(uint64_t *)(addr) != 0xFFFFFFFFFFFFFFFF){  // area is empty
        FLASH_soft_lock();
         __enable_irq();
        return FLASH_ADDRESS_NOT_EMPTY;
    }
#endif

    FLASH->SR |= FLASH_PGERR;  // Reset Error Flags
    FLASH->CR = 0;             // reset CR
    for(uint16_t i = 0; i < word_length; i += OFFSET_ALIGN_BYTES){
        IWDG->KR = 0xAAAA;
        if(FLASH->SR & FLASH_PGERR){
            __enable_irq();
            return FLASH_ERROR_WRITE;
        }
        if (*(uint64_t *)(addr) != data[i]) {
            uint16_t page = (uint32_t)(addr - 0x8000000) >> FLASH_PAGE_2_POW;
            FLASH->CR &= ~(0xFF << FLASH_CR_PNB_Pos);
            FLASH->CR |= (page << FLASH_CR_PNB_Pos) | FLASH_CR_PG;
            *(uint64_t *)(addr + i) = data[i];
        }
        while((FLASH->SR & FLASH_SR_BSY) && (timeout > 0)) timeout--;
        FLASH->CR = 0;
        if(timeout == 0){
            __enable_irq();
            FLASH_soft_lock();
            return FLASH_TIMEOUT;
        }
        FLASH->SR = FLASH_SR_EOP;
    }
    FLASH_soft_lock();
    __enable_irq();
    return FLASH_OK;

}

void FLASH_read(uint8_t page_num, uint16_t offset, uint64_t *buffer, uint16_t length){
    for(uint16_t i = 0; i < length; i++){
        buffer[i] = (*(__IO uint64_t*)(EMBEDDED_FLASH_ADDRESS + page_num * FLASH_PAGE_SIZE + (offset + i) * OFFSET_ALIGN_BYTES));
    }
}

FLASH_status FLASH_erase_page(uint8_t page_num){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    if(!FLASH_UNLOCK())
        return FLASH_LOCKED; // the FLASH is LOCKED
    FLASH->CR &= !(FLASH_CR_PNB | FLASH_CR_MER1);  // clear page number selection and mass erase bit
    FLASH->CR |= (page_num << FLASH_CR_PNB_Pos) | FLASH_CR_PER;
    FLASH->CR |= FLASH_CR_STRT;

    while((!(FLASH->SR & FLASH_SR_EOP)) && (timeout > 0)) timeout--;
	FLASH->SR = FLASH_SR_EOP;

    FLASH_soft_lock();
    return FLASH_OK;
}

FLASH_status FLASH_mass_erase(){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    if(!FLASH_UNLOCK())
        return FLASH_LOCKED; // FLASH is LOCKED
    FLASH->CR &= !(FLASH_CR_PNB | FLASH_CR_PER);
    FLASH->CR |= FLASH_CR_MER1;
    FLASH->CR |= FLASH_CR_STRT;

	while((!(FLASH->SR & FLASH_SR_EOP)) && (timeout > 0)) timeout--;
	FLASH->SR = FLASH_SR_EOP;  // reset End of Op

    FLASH_soft_lock();
    return FLASH_OK;

}

typedef void (*fnc_ptr)(void);

void FLASH_jump_to_app(uint32_t *addr)
{
  /* Function pointer to the address of the user application. */
  fnc_ptr jump_to_app;
  jump_to_app = (fnc_ptr)(*(addr + 4));
//   HAL_DeInit();
  /* Change the main stack pointer. */
  __set_MSP(*addr);
  jump_to_app();
}

uint8_t SetPrefferedBlockNum(uint8_t block_num){
    uint32_t addr;
    uint64_t tmp64;
    uint16_t n;
    uint8_t pref2 = (block_num & 1) ? 1 : 0;
    if ((HavePrefFlashBlockNum() ^ pref2) & 1) {
        /*ищем единичный бит*/
        for (addr = PREF_REC_ADDR_START; addr < PREF_REC_ADDR_END; addr += 4) {
            tmp64 = 1;
            if (M64(addr) != 0) {
                for (n = 0; n < 64; n++) {
                    if (M64(addr) & tmp64) break;
                    tmp64 = tmp64 << 1;
                }
                break;
            }
        }
        if (addr < PREF_REC_ADDR_END) {
            /*зануляем единичный бит*/
            tmp64 = M64(addr) & ~(1 << n);
            if (FLASH_write_addr((uint32_t *)addr, &tmp64, 1) != FLASH_OK) {
                // Flash_Status |= FLASH_STAT_MASK_ERR;
                return 1;
            }
        } else {
            /*ресурс исчерпан*/
            // Flash_Status |= FLASH_STAT_MASK_ERR;
            return 1;
        }
    }
    return 0;
}