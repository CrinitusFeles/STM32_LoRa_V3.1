#include "periph_handlers.h"
// #include "main.h"
#include "adc.h"
// #include "gsm.h"
#include "global_variables.h"



// void USART3_IRQHandler(void) {
//     GSM_RX_Handler();
// }
// void EXTI9_5_IRQHandler(void){
//     EXTI->PR1 |= EXTI_PR1_PIF6;
//     SX126x_RxHandler(&SX1268);

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