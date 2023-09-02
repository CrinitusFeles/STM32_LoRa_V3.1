
#ifndef IWDG_H_
#define IWDG_H_
#include "stm32l4xx.h"

uint16_t IWDG_init(uint16_t period_ms);
void IWDG_refresh();
void IWDG_disable_in_debug();
void IWDG_enable_in_debug();
#endif