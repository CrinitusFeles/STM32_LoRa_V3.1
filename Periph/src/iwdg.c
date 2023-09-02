
#include "iwdg.h"
#include "stm32l4xx.h"
#define LSI_CLK      40000  // 40 KHz

uint16_t IWDG_init(uint16_t period_ms){
    IWDG->KR = 0x0000CCCC;
    IWDG->KR = 0x00005555;
    // if(prescaler > 6)
    //     prescaler = 6;
    IWDG->PR = 6;
    // IWDG->RLR = counter_value & 0x0FFF;
    uint16_t counter = (period_ms * LSI_CLK / 256000) & 0x0FFF;
    IWDG->RLR = counter;
    while(IWDG->SR != 0x00);
    IWDG->KR = 0x0000AAAA;
    return counter;  // (counter & 0x0FFF) * 256 / LSI_CLK
}

void IWDG_refresh(){
    IWDG->KR = 0x0000AAAA;
}

void IWDG_disable_in_debug(){
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_IWDG_STOP;
}

void IWDG_enable_in_debug(){
    DBGMCU->APB1FZR1 &= !DBGMCU_APB1FZR1_DBG_IWDG_STOP;
}