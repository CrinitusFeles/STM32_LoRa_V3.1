
#include "flash.h"
#include "system_select.h"

#define KEY1                        0x45670123
#define KEY2                        0xCDEF89AB
#define OPTKEY1                     0x08192A3B
#define OPTKEY2                     0x4C5D6E7F
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


FLASH_status FLASH_write_page(uint8_t page_num, uint16_t offset, uint64_t *data, uint16_t length){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    if(!FLASH_UNLOCK())
        return FLASH_LOCKED; // FLASH is LOCKED
    uint64_t flash_data = (*(__IO uint64_t*)(FLASH_START_ADDR + page_num * FLASH_PAGE_SIZE + offset * OFFSET_ALIGN_BYTES));
    if(flash_data != 0xFFFFFFFFFFFFFFFF){  // area is empty
        FLASH_soft_lock();
        return FLASH_ADDRESS_NOT_EMPTY;
    }

    FLASH->CR &= !(FLASH_CR_PNB | FLASH_CR_MER1 | FLASH_CR_PER | FLASH_CR_FSTPG);
    FLASH->CR |= (page_num << FLASH_CR_PNB_Pos) | FLASH_CR_PG;
    for(uint16_t i = 0; i < length; i++){
        if(FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR))
            return FLASH_ERROR_WRITE;
        (*(__IO uint64_t*)(FLASH_START_ADDR + page_num * FLASH_PAGE_SIZE + (offset + i) * OFFSET_ALIGN_BYTES)) = data[i];
        while((!(FLASH->SR & FLASH_SR_EOP)) && timeout--) ;
        FLASH->SR = FLASH_SR_EOP;
    }
    FLASH_soft_lock();
    return FLASH_OK;

}

FLASH_status FLASH_write(uint32_t addr, uint64_t *data, uint16_t word_length){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    if(addr < FLASH_START_ADDR || addr >= FLASH_START_ADDR + FLASH_MAX_SIZE){
        return FLASH_INCORRECT_ADDRESS;
    }
    __disable_irq();
    if(FLASH_UNLOCK() != FLASH_OK){
         __enable_irq();
        return FLASH_LOCKED; // FLASH is LOCKED
    }
// #ifndef FLASH_BIT_ACCESS
//     if(*(uint64_t *)(addr) != 0xFFFFFFFFFFFFFFFF){  // area is empty
//         FLASH_soft_lock();
//          __enable_irq();
//         return FLASH_ADDRESS_NOT_EMPTY;
//     }
// #endif

    FLASH->SR |= FLASH_PGERR;  // Reset Error Flags
    FLASH->CR = 0;             // reset CR
    for(uint16_t i = 0; i < word_length; i++){
        IWDG->KR = 0xAAAA;
        if(FLASH->SR & FLASH_PGERR){
            __enable_irq();
            return FLASH_ERROR_WRITE;
        }
        if (M64(addr + i * OFFSET_ALIGN_BYTES) != data[i]) {
            uint16_t page = (uint32_t)(addr - FLASH_START_ADDR) >> FLASH_PAGE_2_POW;
            FLASH->CR &= ~(0xFF << FLASH_CR_PNB_Pos);
            FLASH->CR |= (page << FLASH_CR_PNB_Pos) | FLASH_CR_PG;
            M64(addr + i * OFFSET_ALIGN_BYTES) = data[i];
        }
        while((FLASH->SR & FLASH_SR_BSY) && timeout--) ;
        FLASH->CR = 0;
        if(timeout == 0){
            __enable_irq();
            FLASH_soft_lock();
            return FLASH_TIMEOUT;
        }
        FLASH->SR = FLASH_SR_EOP;
        if(FLASH->SR != 0){
            break;
        }
    }
    FLASH_soft_lock();
    __enable_irq();
    if(FLASH->SR != 0){
        return FLASH_ERROR_WRITE;
    }
    return FLASH_OK;

}

void FLASH_read(uint8_t page_num, uint16_t offset, uint64_t *buffer, uint16_t length){
    for(uint16_t i = 0; i < length; i++){
        buffer[i] = (*(__IO uint64_t*)(FLASH_START_ADDR + page_num * FLASH_PAGE_SIZE + (offset + i) * OFFSET_ALIGN_BYTES));
    }
}

FLASH_status FLASH_erase_page(uint8_t page_num){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    if(FLASH_UNLOCK() != FLASH_OK)
        return FLASH_LOCKED; // the FLASH is LOCKED
    FLASH->CR &= !(FLASH_CR_PNB | FLASH_CR_MER1);  // clear page number selection and mass erase bit
    FLASH->CR |= (page_num << FLASH_CR_PNB_Pos) | FLASH_CR_PER;
    FLASH->CR |= FLASH_CR_STRT;

    while((FLASH->SR & FLASH_SR_BSY) && (timeout > 0)) timeout--;
	FLASH->SR = FLASH_SR_EOP;

    FLASH_soft_lock();
    return FLASH_OK;
}

FLASH_status FLASH_mass_erase(){
    uint32_t timeout = FLASH_OPERATION_TIMEOUT;
    if(FLASH_UNLOCK() != FLASH_OK)
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

void FLASH_jump_to_app(uint32_t addr)
{
  /* Function pointer to the address of the user application. */
  fnc_ptr jump_to_app;
  jump_to_app = (fnc_ptr)(*(uint32_t *)(addr + 4));
//   HAL_DeInit();
  /* Change the main stack pointer. */
  __set_MSP(*(uint32_t*)addr);
  jump_to_app();
}

FLASH_status SetPrefferedBlockNum(uint8_t block_num){
    uint32_t addr;
    uint8_t pref2 = (block_num & 1) ? 1 : 0;
    FLASH_status status;
    uint64_t val = 1;
    uint64_t val0 = 0;
    if ((HavePrefFlashBlockNum() ^ pref2) & 1) {
        /*ищем единичный бит*/
        for (addr = PREF_REC_ADDR_START; addr < PREF_REC_ADDR_END; addr += OFFSET_ALIGN_BYTES) {
            if (M64(addr) > 0) {
                status = FLASH_write(addr, M64(addr) == (uint64_t)(-1) ? &val : &val0, 1);
                return status;
            }
        }
        return FLASH_FULL;
    }
    return FLASH_OK;
}


FLASH_status FLASH_erase_firmware(uint8_t current_block_num){
    uint8_t start_page = current_block_num == 0 ? FW_PAGES_AMOUNT : 0;
    for (uint8_t i = start_page; i < FW_PAGES_AMOUNT - 1 + start_page; i++) {
        FLASH_status status = FLASH_erase_page(i);
        if(status != FLASH_OK) return status;
    }
    return FLASH_OK;
}

void FLASH_disable_iwdg_stby(){
    uint32_t option_bytes = *(uint32_t *)0x1FFF7800;
    if(option_bytes & FLASH_OPTR_IWDG_STDBY){
        __disable_irq();
        if(FLASH_UNLOCK() != FLASH_OK){
            __enable_irq();
        }
        FLASH->OPTKEYR = OPTKEY1;
        FLASH->OPTKEYR = OPTKEY2;
        FLASH_wait_operation();
        FLASH->OPTR &= ~FLASH_OPTR_IWDG_STDBY;
        FLASH->CR |= FLASH_CR_OPTSTRT;
        FLASH_wait_operation();
        FLASH_soft_lock();
        __enable_irq();
    }
}

bool FLASH_erase_neighbor(){
    uint8_t curr_block = HaveRunFlashBlockNum();
    if(curr_block){
        for(uint8_t sec_num = 0; sec_num < 63; sec_num++){
            if(FLASH_erase_page(sec_num) != FLASH_OK) {
                return false;
            }
        }
    } else {
        for(uint8_t sec_num = 64; sec_num < 127; sec_num++){
            if(FLASH_erase_page(sec_num) != FLASH_OK) {
                return false;
            }
        }
    }
    return true;
}