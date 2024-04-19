#include "formating_output.h"
#include "main.h"


// setvbuf(stdout, NULL, _IONBF, 0);  - for disabling bufferization or you need
// to '/n' at every printf()
#define UNUSED(x) (void)(x)

int _write(int file, char *ptr, int len){
    UNUSED(file);
	for(int i = 0; i < len; i++){
#ifndef USART_PRINT
		ITM_SendChar((*ptr++));
#else
		UART_tx(USART1, (ptr[i]));
#endif
    }
	return len;
}

void print_arr(char *arr){
	for(int i = 0; arr[i] != '\0'; i++){
		ITM_SendChar(arr[i]);
	}
}
