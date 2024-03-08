
#include "wwdg.h"
#include "stm32l4xx.h"
#define t_PCLK      4000  // 4 MHz

uint16_t WWDG_init(uint8_t timer_base, uint8_t window_value){
    uint16_t period_ms = 4096 * (2 << timer_base) * ((window_value & 0x3F) + 1) / t_PCLK;
    WWDG->CR = 0x7F & window_value;
    WWDG->CFR = 0x7F & (window_value - 1);
    WWDG->CFR |= (timer_base & 0x03) << WWDG_CFR_WDGTB_Pos;
    WWDG->CR |= WWDG_CR_WDGA;
    return period_ms;
}

// void WWDG_refresh(uint8_t window_value){

// }

void WWDG_disable_in_debug(){
    DBGMCU->APB1FZR1 |= DBGMCU_APB1FZR1_DBG_WWDG_STOP;
}

void WWDG_enable_in_debug(){
    DBGMCU->APB1FZR1 &= !DBGMCU_APB1FZR1_DBG_WWDG_STOP;
}