#ifndef INC_PERIPH_HANDLERS_H_
#define INC_PERIPH_HANDLERS_H_
#include "stm32l4xx.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "semphr.h"


// void EXTI4_IRQHandler();
void USART1_IRQHandler(void);
void USART3_IRQHandler(void);
void LPUART1_IRQHandler(void);
// void TIM7_IRQHandler(void);
void RTC_Alarm_IRQHandler(void);
void ADC1_IRQHandler(void);
void EXTI9_5_IRQHandler(void);

extern StreamBufferHandle_t  gsm_stream;
extern StreamBufferHandle_t  cli_stream;
extern  SemaphoreHandle_t xSemaphore;


#endif /* INC_PERIPH_HANDLERS_H_ */
