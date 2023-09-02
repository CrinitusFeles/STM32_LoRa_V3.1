/*
 * formating_output.h
 *
 *  Created on: 20 мар. 2020 г.
 *      Author: BreakingBad
 */

#ifndef CODE_HEADER_FORMATING_OUTPUT_H_
#define CODE_HEADER_FORMATING_OUTPUT_H_
#include "stm32l4xx.h"
#include <stdio.h>
//#include "uart.h"
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

int _write(int file, char *ptr, int len);
void print_arr(char *arr);

#endif /* CODE_HEADER_FORMATING_OUTPUT_H_ */
