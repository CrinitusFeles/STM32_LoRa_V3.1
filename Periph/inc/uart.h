/*
 * uart.h
 *
 *  Created on: 3 ����. 2020 �.
 *      Author: BilboBaggins
 */

#ifndef CODE_HEADER_UART_H_
#define CODE_HEADER_UART_H_

#include "stm32l4xx.h"

extern uint8_t uart_receive_data;
enum duplex_mode{
    FULL_DUPLEX, HALF_DUPLEX
};

/// @brief Инициализация UART
/// @param UARTx Указатель на структуру UART (USART1, USART2, USART3)
/// @param baudrate Любое положительное значение (9600, 115200)
/// @param duplex_mode Режим дуплекса (FULL_DUPLEX, HALF_DUPLEX)
void UART_init(USART_TypeDef *UARTx, uint32_t baudrate, uint8_t duplex_mode);
void UART_tx(USART_TypeDef *UARTx, uint8_t data);
void UART_tx_string(USART_TypeDef *UARTx, char *array);

#endif /* CODE_HEADER_UART_H_ */
