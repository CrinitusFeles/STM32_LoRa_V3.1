#ifndef INC_PERIPH_HANDLERS_H_
#define INC_PERIPH_HANDLERS_H_
#include "stm32l4xx.h"

// void EXTI4_IRQHandler();
// void USART1_IRQHandler(void);
void USART3_IRQHandler(void);
// void TIM7_IRQHandler(void);
void RTC_Alarm_IRQHandler(void);
void ADC1_IRQHandler(void);
void EXTI9_5_IRQHandler(void);
#endif /* INC_PERIPH_HANDLERS_H_ */
