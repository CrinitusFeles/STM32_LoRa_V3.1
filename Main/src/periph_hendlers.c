#include "periph_handlers.h"
#include "uart.h"
#include "adc.h"
#include "gsm.h"
#include <string.h>
#include "sx127x.h"
// #include "sx126x.h"
#include "microrl.h"
#include "stm32_misc.h"
#include "xprintf.h"

GSM sim7000g;


void LPUART1_IRQHandler(void) {
    // BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    while (LPUART1->ISR & USART_ISR_RXNE) {
        char data = LPUART1->RDR;
        // sim7000g.rx_buf[sim7000g.rx_counter] = data;
        xStreamBufferSendFromISR(gsm_stream, ( void * )&data, 1, NULL);
        // sim7000g.rx_counter++;
        // if(sim7000g.rx_counter >= sizeof(sim7000g.rx_buf)){
        //     sim7000g.rx_counter = 0;
        //     sim7000g.status.buffer_filled = 1;
        // }
	}
    if(LPUART1->ISR & USART_ISR_IDLE){
        LPUART1->ICR |= USART_ICR_IDLECF;
        // for(uint32_t i = 0; i < 40000; i++){
        //     if (LPUART1->ISR & USART_ISR_RXNE) return;
        // }
        // GSM_AnswerParser(); // TODO: вызывать парсер после каждой функции GSM_wait_for_answer(), а не в прерывании
                            // тогда не нужно будет тупить десятки тысяч циклов в самом начале парсера
    }
	if(LPUART1->ISR & USART_ISR_ORE){
        (void)LPUART1->RDR;
        sim7000g.overrun_counter++;
		LPUART1->ICR |= USART_ICR_ORECF;
		// UART_tx_array(USART1, "USART3 OVERRUN ERROR!\r\n");
	}
    if(LPUART1->ISR & USART_ISR_FE){
        LPUART1->ICR |= USART_ICR_FECF;
        sim7000g.status.pwr_status = 0;
        sim7000g.frame_error_counter++;
    }
}
void USART1_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    while (USART1->ISR & USART_ISR_RXNE) {
        char data = USART1->RDR;
        xStreamBufferSendFromISR(cli_stream, ( void * )&data, 1, &xHigherPriorityTaskWoken);

	}
    xdev_out(uart_print);
    if(USART1->ISR & USART_ISR_IDLE){
        USART1->ICR |= USART_ICR_IDLECF;
    //     for(; processed_symbols < buffer_size; processed_symbols++){
    //         if(USART1->ISR & USART_ISR_RXNE){
    //             return;
    //         }
    //         microrl_insert_char(prl, (int)(buffer[processed_symbols]));
    //     }
    //     memset(buffer, 0, buffer_size);
    //     buffer_size = 0;
    //     processed_symbols = 0;
    }
	if(USART1->ISR & USART_ISR_ORE){
        (void)(USART1->RDR);
		USART1->ICR |= USART_ICR_ORECF;
		// UART_tx_string(USART1, "\n\rOVERRUN ERROR!\r\n");
	}
    if(USART1->ISR & USART_ISR_FE){
        USART1->ICR |= USART_ICR_FECF;
        // UART_tx_string(USART1, "\n\rFRAMING ERROR!\r\n");
    }
}
// void USART3_IRQHandler(void) {
//     GSM_RX_Handler();
// }
void EXTI9_5_IRQHandler(void){
    EXTI->PR1 |= EXTI_PR1_PIF6;
    sx127x.new_rx_data_flag = 1;
}

// void EXTI2_IRQHandler(void){
//     EXTI->PR1 |= EXTI_PR1_PIF2;
//     SX1268.new_rx_data_flag = 1;
// }
void ADC1_IRQHandler(void){
    ADC_Handler();
}
void RTC_WKUP_IRQHandler(){
    EXTI->PR1 |= EXTI_PR1_PIF20;	// clear pending flag (WAKEUP)
    PWR->CR1 |= PWR_CR1_DBP;
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

    RTC->ISR &= ~RTC_ISR_WUTF;
    RTC->WPR = 0xFF;
	PWR->CR1 &= ~PWR_CR1_DBP;
}

void RTC_Alarm_IRQHandler(void)
{
	EXTI->PR1 |= EXTI_PR1_PIF18;	// clear pending flag (ALARM)

	// // unlock write protection
	PWR->CR1 |= PWR_CR1_DBP;
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

	// // disable Alarm A
	// RTC->CR &= ~RTC_CR_ALRAE;

	// // wait for Alarm A write flag, to make sure the access to alarm reg is allowed
	// while (!(RTC->ISR & RTC_ISR_ALRAWF));

	// RTC->ALRMAR &= (RTC_ALRMAR_DU ^ RTC_ALRMAR_DU);		// Bits 27:24 DU[3:0]: Date units or day in BCD format.
	// RTC->ALRMAR &= (RTC_ALRMAR_HT ^ RTC_ALRMAR_HT);		// Bits 21:20 HT[1:0]: Hour tens in BCD forma
	// RTC->ALRMAR &= (RTC_ALRMAR_HU ^ RTC_ALRMAR_HU);		// Bits 19:16 HU[3:0]: Hour units in BCD format.
	// RTC->ALRMAR &= (RTC_ALRMAR_MNT ^ RTC_ALRMAR_MNT);	// Bits 14:12 MNT[2:0]: Minute tens in BCD format.
	// RTC->ALRMAR &= (RTC_ALRMAR_MNU ^ RTC_ALRMAR_MNU);	// Bits 11:8 MNU[3:0]: Minute units in BCD format.

	// // lock write protection - writing a wrong key reactivates the write protection
	RTC->ISR &= ~RTC_ISR_ALRAF;	//  flag is cleared by software by writing 0

    RTC->WPR = 0xFF;
	PWR->CR1 &= ~PWR_CR1_DBP;


	// UART_tx_array(USART1, "RTC handler\r\n");
}