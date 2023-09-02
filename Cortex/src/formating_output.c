#include "formating_output.h"

int _write(int file, char *ptr, int len)
{
	for(int i = 0 ; i < len ; i++)
		ITM_SendChar((*ptr++));
//		UART_tx((*ptr++));
	return len;
}

void print_arr(char *arr){
	for(int i = 0; arr[i] != '\0'; i++){
		ITM_SendChar(arr[i]);
	}
}
