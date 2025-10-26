/*
 * delay.h
 *
 *  Created on: 20 ���. 2020 �.
 *      Author: ftor
 */

#ifndef CODE_DELAY_H_
#define CODE_DELAY_H_
#include "stm32l431xx.h"
#include "System.h"

#define millisec F_CPU/1000 - 1 //milisec
#define microsec  F_CPU/1000000-1

// void SysTick_Handler();

void Delay(uint32_t milli);
void Freeze_delay(uint32_t milli);
void MicroDelay(uint32_t micro);
uint32_t GetMili();
uint32_t GetMicro();
void delay_action(uint32_t milli, uint8_t process_num, void (*do_action)());

#endif /* CODE_DELAY_H_ */
