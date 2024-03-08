#include "delay.h"
#include "iwdg.h"
static volatile uint32_t count = 0;
static volatile uint32_t delay_count = 0;
volatile uint32_t iwdg_reset = 0;

// #ifndef INC_FREERTOS_H
// void SysTick_Handler(){
// 	delay_count++;
//     count++;
//     IWDG_refresh();
//     // if(delay_count - iwdg_reset > 100){

//     //     iwdg_reset = delay_count;
//     // }
// }
// #endif
void Delay(uint32_t milli){
	delay_count = 0;
	while(delay_count < milli);
	delay_count = 0;
}
void Freeze_delay(uint32_t milli){
	for(uint32_t counter = 8000 * milli; counter != 0; counter--);
}
uint32_t GetMicro(){
	return count;
}
uint32_t GetMili(){
	return count;
}

void delay_action(uint32_t milli, uint8_t process_num, void (*do_action)()){
	static uint8_t flag[8] = {0};
	static uint32_t counter[8] = {0};

	if(!flag[process_num]){
		counter[process_num] = GetMili();
		flag[process_num] = 1;
	}
	else{
		if(GetMili() - counter[process_num] > milli){
			counter[process_num] = GetMili();
			do_action();
			flag[process_num] = 0;
		}
	}

}
