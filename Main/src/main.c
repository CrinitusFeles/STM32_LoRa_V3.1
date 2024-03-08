#include "main.h"
#include "low_power.h"
#include "string.h"
#include "System.h"
#include "FreeRTOS.h"
#include "task.h"
#include "fifo.h"
#include "monitor_task.h"

FIFO fifo;

void vConfigureTimerForRunTimeStats(void){
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM7EN;
    TIM7->PSC = F_CPU / 1000000 - 1;
    TIM7->ARR = 0xFFFF;
    TIM7->CR1 |= TIM_CR1_CEN;
}
unsigned long vGetTimerForRunTimeStats(void){
    // TIM7->SR &= ~TIM_SR_UIF;
    // TIM7->DIER &= ~TIM_DIER_UIE;
    ulHighFrequencyTimerTicks += TIM7->CNT;
    TIM7->ARR = 0xFFFF;
    TIM7->CNT = 0;
    /* Increment the counter used to mease execution time */
    return ulHighFrequencyTimerTicks;
}


void LED_BLINK(void *pvParameters){
    for (;;) {
        gpio_toggle(LED);
        vTaskDelay(500);
    }
    vTaskDelete( NULL );
	// Delay(1000);
}

void PERIPH_TOGGLE(void *pvParameters){
    for (;;) {
        gpio_toggle(EN_SD);
        vTaskDelay(1500);
    }
    vTaskDelete( NULL );
	// Delay(1000);
}
void CONSOLE_TASK(void *pvParameters){
    for (;;) {
        if(!FIFO_IS_EMPTY(fifo)){
            if(prl->last_index < 50){
                prl->last_index += 1;
            } else {
                prl->last_index = 0;
            }
            prl->buffer[prl->last_index] = FIFO_FRONT(fifo);
            FIFO_POP(fifo);

            if(prl->last_index != prl->current_index){
                if(prl->current_index < 50){
                    prl->current_index += 1;
                } else {
                    prl->current_index = 0;
                }
                microrl_insert_char(prl, (int)(prl->buffer[prl->current_index]));
            }
        }
        vTaskDelay(1);
    }
    vTaskDelete( NULL );
	// Delay(1000);
}

int main(){
    System_Init();
    // xTaskCreate( MonitorTask, "TRACE", configMINIMAL_STACK_SIZE, NULL, 1, ( xTaskHandle * ) NULL);
    xTaskCreate( LED_BLINK, "LED1", configMINIMAL_STACK_SIZE, NULL, 2, ( xTaskHandle * ) NULL);
    xTaskCreate( PERIPH_TOGGLE, "PERIPH_TOGGLE", configMINIMAL_STACK_SIZE, NULL, 2, ( xTaskHandle * ) NULL);
    xTaskCreate( CONSOLE_TASK, "CONSOLE", configMINIMAL_STACK_SIZE * 2, NULL, 2, ( xTaskHandle * ) NULL);
    vTaskStartScheduler();
    // while(1){
        // Delay(1000);
        // delay_action(1000, 1, LED_BLINK);

        // stop_cortex();
    // }
}


