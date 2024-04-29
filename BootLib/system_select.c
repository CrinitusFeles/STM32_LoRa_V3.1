#include "stm32l431xx.h"
#include "system_select.h"
#include "flash.h"


// int8_t CheckBlock(register uint32_t *addr) __attribute__((section("!!!!my_system_init_sect")));
// uint8_t HavePrefFlashBlockNum() __attribute__((section("!!!!my_system_init_sect")));
// uint8_t HaveRunFlashBlockNum() __attribute__((section("!!!!my_system_init_sect")));
// void SystemSelect() __attribute__((section("!!!!my_system_init_sect")));


uint8_t HavePrefFlashBlockNum() {  // return: 0 - 1-й блок,  1 - 2-й блок
    register uint32_t *addr;
    register uint64_t word;
    uint8_t num = 0;
    for (addr = (uint32_t *)PREF_REC_ADDR_START; addr < (uint32_t *)PREF_REC_ADDR_END; addr++) {
        word = *addr;
        if (word == 0xFFFFFFFFFFFFFFFF) {
            break;
        }
        num += (word == 1) ? 1 : 0;
    }
    return (num & 1);
}

uint8_t HaveRunFlashBlockNum() {
    register uint32_t pc;
    __ASM volatile("MOV %0, PC\n" : "=r"(pc));
    return pc > RESERVE_FW_ADDR ? 1 : 0;
}

int8_t CheckBlock(register uint32_t *addr) {
    register uint32_t ptr, rcc_save;
    int8_t ret = -1;  // incorrect crc
    if (*addr < 0x40000000) {  // указатель стека должен быть таким (при отсутствии прошивки будет 0xFFFFFFFFFF)
        if (((*(addr + 1) ^ (uint32_t)addr) & 0x20000) == 0) {  // для того ли блока этот образ?
            rcc_save = RCC->AHB1ENR;
            RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
            CRC->CR = 1;
            for (ptr = 0; ptr < 0x20000 / 4 - 1; ptr++) {
                CRC->DR = *addr++;
                if(ptr == 0x1cb0){
                    ptr = 0x1cb0;
                }
                if (CRC->DR == 0){
                    if(*addr == (ptr + 1)) {  // последнее слово должно совпадать с оффсетом
                        ret = 0;
                        break;
                    }
                }
            }
            RCC->AHB1ENR = rcc_save;
        } else {
            return -2;  // incorrect block
        }
    } else {
        return -3;  // FLASH is empty
    }
    return ret;
}


void SystemSelect() {
    if (HaveRunFlashBlockNum() != 0) {
        /*мы во 2-м блоке*/
        SystemInit();
        SCB->VTOR = RESERVE_FW_ADDR;
        return;
    }
    if (CheckBlock((uint32_t *)RESERVE_FW_ADDR) == 0) {
        if (CheckBlock((uint32_t *)MAIN_FW_ADDR) == 0) {
            if (HavePrefFlashBlockNum() != 0) {
                if ((RCC->CSR & (RCC_CSR_WWDGRSTF | RCC_CSR_IWDGRSTF)) == 0){
                    FLASH_jump_to_app(RESERVE_FW_ADDR);
                }
            }
        } else {
            FLASH_jump_to_app(RESERVE_FW_ADDR);
        }
    }
    SystemInit();
    return;
}
