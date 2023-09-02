
#include "flash.h"

#define KEY1                        0x45670123
#define KEY2                        0xCDEF89AB
#define EMBEDDED_FLASH_ADDRESS      0x08000000
#define FLASH_PAGE_SIZE             2048
#define FLASH_OPERATION_TIMEOUT     1000
#define OFFSET_ALIGN_BYTES          8 // 64 bit word

void FLASH_wait_operation(){
    for(uint16_t timeout = 0; (FLASH->SR & FLASH_SR_BSY) && (timeout < FLASH_OPERATION_TIMEOUT); timeout++);
    if (FLASH->SR & FLASH_SR_EOP) {
		FLASH->SR = FLASH_SR_EOP;
	}
}

uint8_t FLASH_UNLOCK(){
    FLASH->KEYR = KEY1;
    FLASH->KEYR = KEY2;
    FLASH_wait_operation();
    if(FLASH->CR & FLASH_CR_LOCK)
        return 0;  // the FLASH is LOCKED
    return 1;
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


uint8_t FLASH_write(uint8_t page_num, uint16_t offset, uint64_t *data, uint16_t length){
    if(!FLASH_UNLOCK())
        return 0; // the FLASH is LOCKED
    uint64_t flash_data = (*(__IO uint64_t*)(EMBEDDED_FLASH_ADDRESS + page_num * FLASH_PAGE_SIZE + offset * OFFSET_ALIGN_BYTES));
    if(flash_data != 0xFFFFFFFFFFFFFFFF){  // area is empty
        FLASH_soft_lock();
        return 0;
    }

    FLASH->CR &= !(FLASH_CR_PNB | FLASH_CR_MER1 | FLASH_CR_PER | FLASH_CR_FSTPG);
    FLASH->CR |= (page_num << FLASH_CR_PNB_Pos) | FLASH_CR_PG;
    for(uint16_t i = 0; i < length; i++){
        if(FLASH->SR & (FLASH_SR_PROGERR | FLASH_SR_PGAERR | FLASH_SR_PGSERR))
            return 0;
        (*(__IO uint64_t*)(EMBEDDED_FLASH_ADDRESS + page_num * FLASH_PAGE_SIZE + (offset + i) * OFFSET_ALIGN_BYTES)) = data[i];
        for(uint16_t timeout = 0; (!(FLASH->SR & FLASH_SR_EOP)) && (timeout < FLASH_OPERATION_TIMEOUT); timeout++);
        FLASH->SR = FLASH_SR_EOP;
    }
    FLASH_soft_lock();
    return 1;

}

void FLASH_read(uint8_t page_num, uint16_t offset, uint64_t *buffer, uint16_t length){
    for(uint16_t i = 0; i < length; i++){
        buffer[i] = (*(__IO uint64_t*)(EMBEDDED_FLASH_ADDRESS + page_num * FLASH_PAGE_SIZE + (offset + i) * OFFSET_ALIGN_BYTES));
    }
}

uint8_t FLASH_erase_page(uint8_t page_num){
    if(!FLASH_UNLOCK())
        return 0; // the FLASH is LOCKED
    FLASH->CR &= !(FLASH_CR_PNB | FLASH_CR_MER1);  // clear page number selection and mass erase bit
    FLASH->CR |= (page_num << FLASH_CR_PNB_Pos) | FLASH_CR_PER;
    FLASH->CR |= FLASH_CR_STRT;

    for(uint16_t timeout = 0; (!(FLASH->SR & FLASH_SR_EOP)) && (timeout < FLASH_OPERATION_TIMEOUT); timeout++);
	FLASH->SR = FLASH_SR_EOP;

    FLASH_soft_lock();
    return 1;
}

uint8_t FLASH_mass_erase(){
    if(!FLASH_UNLOCK())
        return 0; // the FLASH is LOCKED
    FLASH->CR &= !(FLASH_CR_PNB | FLASH_CR_PER);
    FLASH->CR |= FLASH_CR_MER1;
    FLASH->CR |= FLASH_CR_STRT;

	for(uint16_t timeout = 0; (!(FLASH->SR & FLASH_SR_EOP)) && (timeout < FLASH_OPERATION_TIMEOUT); timeout++);
	FLASH->SR = FLASH_SR_EOP;  // reset End of Op

    FLASH_soft_lock();
    return 1;

}

