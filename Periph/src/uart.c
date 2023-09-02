#include "stm32l431xx.h"
#include "uart.h"

#define APB2_CLK 4000000
#define APB1_CLK 4000000
// TODO: добавить в библиотеку rcc.h расчет тактовой частоты RCC_GetPCLK1Freq() RCC_GetPCLK2Freq()
// путем вычитывания значений множителей регистров тактирования
void UART_init(USART_TypeDef *USARTx, uint32_t baudrate, uint8_t duplex_mode){
	/* gpio init example
	gpio_init(USART1_RX, PA10_USART1_RX, Open_drain, no_pull, Input);
	gpio_init(USART1_TX, PA9_USART1_TX, Push_pull, no_pull, High_speed);
	*/
	USARTx->CR1 = 0;
	USARTx->CR2 = 0;
	USARTx->CR3 = 0;
    USARTx->ICR = 0xFFFF;


	if(USARTx == USART1){
		RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
		NVIC_EnableIRQ(USART1_IRQn);
		USARTx->BRR = APB2_CLK / baudrate;
	}
	else if(USARTx == USART2){
		RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
		NVIC_EnableIRQ(USART2_IRQn);
		USARTx->BRR = APB1_CLK / baudrate;
	}
	else if(USARTx == USART3){
		RCC->APB1ENR1 |= RCC_APB1ENR1_USART3EN;
		NVIC_EnableIRQ(USART3_IRQn);
		USARTx->BRR = APB1_CLK / baudrate;
	}
	else if(USARTx == LPUART1){
		RCC->APB1ENR2 |= RCC_APB1ENR2_LPUART1EN;
		NVIC_EnableIRQ(LPUART1_IRQn);
		USARTx->BRR = APB1_CLK / baudrate;
	}
	else return;

	USARTx->CR1 &= ~(0x01 << 28); //word length 8 bit
	USARTx->CR1 &= ~(0x01 << 12); //word length 8 bit

	USARTx->CR1 &= ~(0x01 << 10); //parity disable
//	USARTx->CR1 |= parity_control_ << 10;

	USARTx->CR2 &= ~(0x03 << 12); //1 stop bit
//	USARTx->CR2 |= stop_bits_ << 12;
	if(duplex_mode==HALF_DUPLEX){
		// SCEN, LINEN, SCEN, IREN must be reset
		USARTx->CR2 &= ~(0x01 << USART_CR2_LINEN_Pos);
		USARTx->CR2 &= ~(0x01 << USART_CR2_CLKEN_Pos);
		USARTx->CR3 &= ~(0x01 << USART_CR3_SCEN_Pos);
		USARTx->CR3 &= ~(0x01 << USART_CR3_IREN_Pos);
		// set duplex mode
		USARTx->CR3 &= ~(0x01 << USART_CR3_HDSEL_Pos);
		USARTx->CR3 |= duplex_mode << USART_CR3_HDSEL_Pos;
	}
	else{
		USARTx->CR1 |= USART_CR1_RXNEIE; // RXNE interrupt ON
	}
    USARTx->CR1 |= USART_CR1_TE | USART_CR1_RE;
	USARTx->CR1 |= USART_CR1_UE; // enable USART

}
void UART_tx(USART_TypeDef *USARTx, uint8_t data){
    int16_t timeout = 10000;
	while(!(USARTx->ISR & USART_ISR_TC) && timeout--);
	USARTx->TDR = data;
}

void UART_tx_string(USART_TypeDef *USARTx, char *array){
	for(uint8_t i = 0; array[i] != '\0'; i++) {
		UART_tx(USARTx, array[i]);
	}
}

