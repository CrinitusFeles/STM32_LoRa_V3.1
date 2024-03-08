#ifndef WWDG_H_
#define WWDG_H_
#include "stm32l4xx.h"

uint16_t WWDG_init(uint8_t prescaler, uint8_t window_value);
// void WWDG_refresh(uint8_t window_value);
void WWDG_disable_in_debug();
void WWDG_enable_in_debug();
#endif