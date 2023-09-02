/*
 * dwt.h
 *
 *  Created on: 19 сент. 2020 г.
 *      Author: ftor
 */

#ifndef INC_DWT_H_
#define INC_DWT_H_

#include "stm32l431xx.h"
#include "System.h"

#define DWT_IN_MICROSEC (F_CPU/1000000)

void DWT_Init();
//void DWT_Delay_tics(uint32_t tics);  // dwt tics
void DWT_Delay_us(uint32_t us);      // microseconds
uint32_t DWT_GetMicro();
void DWT_Delay_ms(uint32_t ms);      // milliseconds
void DWT_Delay_With_Action(uint32_t us, int (*cond)(), void (*act)()); // microseconds
void DWT_Delay_Action(uint32_t us, void (*act)());
uint32_t DWT_Get_Current_Tick();
uint32_t DWT_GetDelta(uint32_t t0);
uint32_t DWT_Elapsed_Tick(uint32_t t0);

#endif /* INC_DWT_H_ */
