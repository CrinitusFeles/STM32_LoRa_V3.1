#include "main.h"
#include <stdarg.h>
#include "low_power.h"
#include "string.h"
#include "System.h"
#include "FreeRTOS.h"
#include "task.h"

#include "monitor_task.h"
#include "xprintf.h"
#include "iwdg.h"
#include "stm32_misc.h"
#include "periph_handlers.h"
#include "action_task.h"
#include "system_config.h"
#include "radio_protocol.h"
#include "LoRa.h"


#define UNUSED(x) (void)(x)
#define CONSOLE_SIZE            configMINIMAL_STACK_SIZE * 10
#define RADIO_SIZE              configMINIMAL_STACK_SIZE * 10

#ifdef USE_GSM
#define GSM_SIZE                configMINIMAL_STACK_SIZE
#include "gsm.h"
StreamBufferHandle_t  gsm_stream;
StaticTask_t xTaskBuffer_RADIO_GSM_PRINT;
StackType_t xStack_GSM_PRINT [GSM_SIZE];
SemaphoreHandle_t xSemaphore;
#endif

// uint8_t timeout_counter = 0;
StreamBufferHandle_t  cli_stream;
StreamBufferHandle_t  radio_tx_stream;

StaticTask_t xTaskBuffer_RX_RADIO;
StaticTask_t xTaskBuffer_TX_RADIO;
StaticTask_t xTaskBuffer_RADIO_CONSOLE;
StackType_t xStack_RX_RADIO [RADIO_SIZE];
StackType_t xStack_TX_RADIO [RADIO_SIZE];
StackType_t xStack_CONSOLE [CONSOLE_SIZE];


void route_cli_to_lora(uint8_t data){
    xStreamBufferSend(radio_tx_stream, &data, 1, portMAX_DELAY);
}

void vConfigureTimerForRunTimeStats(void){
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM7EN;
    TIM7->PSC = F_CPU / 1000000 - 1;
    TIM7->ARR = 0xFFFF;
    TIM7->CR1 |= TIM_CR1_CEN;
}

unsigned long vGetTimerForRunTimeStats(void){
    // TIM7->SR &= ~TIM_SR_UIF;
    // TIM7->DIER &= ~TIM_DIER_UIE;
    static volatile unsigned long ulHighFrequencyTimerTicks = 0;
    ulHighFrequencyTimerTicks += TIM7->CNT;
    TIM7->ARR = 0xFFFF;
    TIM7->CNT = 0;
    /* Increment the counter used to mease execution time */
    return ulHighFrequencyTimerTicks;
}

void TX_RADIO_TASK(void *pvParameters){
    UNUSED(pvParameters);
    for (;;) {
        uint8_t size = xStreamBufferReceive(
            radio_tx_stream,
            LoRa.tx_data.payload + LoRa.tx_data.payload_len,
            RADIO_PROTOCOL_PAYLOAD_SIZE - LoRa.tx_data.payload_len, 100
        );
        LoRa.tx_data.payload_len += size;
        if((size == 0 && LoRa.tx_data.payload_len > 0)
            || (size == RADIO_PROTOCOL_PAYLOAD_SIZE || LoRa.tx_data.payload_len == RADIO_PROTOCOL_PAYLOAD_SIZE)){
            LoRa.tx_data.crc16 = crc16_calc(LoRa.tx_data.payload,
                                            LoRa.tx_data.payload_len);
            LoRa.tx_data.src_addr = system_config.module_id;
            LoRa.tx_data.dst_addr = LoRa.rx_data.src_addr;
            LoRa.tx_data.cmd_type = size == 0 ? LORA_ANSWER_LAST : LORA_ANSWER;
            LoRa_Transmit(LoRa.tx_data.buffer, LoRa.tx_data.payload_len + 6);
            LoRa.tx_data.payload_len = 0;
        }
    }
    vTaskDelete( NULL );
}

void RX_RADIO_TASK(void *pvParameters){
    UNUSED(pvParameters);
    char buffer[512] = {0};
    uint16_t j = 0;
    for (;;) {
        if(LoRa.new_rx_data_flag){
            LoRa_RxHandler();
            LoRa.new_rx_data_flag = 0;
            if(LoRa.rx_data.cmd_type < LORA_CMD_REQUEST){
                for(uint8_t i = 0; i < LoRa.rx_data.payload_len; i++){
                    buffer[j] = LoRa.rx_data.payload[i];
                    if(LoRa.rx_data.payload[i] == '\n' && LoRa.rx_data.payload[i-1] == '\r'){
                        buffer[j+1] = 0;
                        UART_tx_string(USART_PRINT, buffer);
                        j = 0;
                        continue;
                    }
                    j++;
                }
                if(LoRa.rx_data.cmd_type == LORA_ANSWER_LAST){
                    buffer[j + 1] = 0;
                    UART_tx_string(USART_PRINT, buffer);
                    j = 0;
                }
                // xprintf((char*)LoRa.rx_data.payload);
                continue;
            }
            if(LoRa.rx_data.dst_addr != system_config.module_id){
                continue;
            }
            uint16_t crc16 = crc16_calc(LoRa.rx_data.payload, LoRa.rx_data.payload_len);
            if(crc16 != LoRa.rx_data.crc16){
                // xprintf((char*)LoRa.rx_data.buffer);
                continue;
            }
            LoRa.rx_data.payload[LoRa.rx_data.payload_len] = 0;
            xdev_out(uart_print);
            xprintf((char*)LoRa.rx_data.payload);
            xdev_out(route_cli_to_lora);
            xStreamBufferSend(cli_stream, &LoRa.rx_data.payload,
                              LoRa.rx_data.payload_len, portMAX_DELAY);
            // xStreamBufferSend(cli_stream, "\n\r", 3, portMAX_DELAY);
        }
        vTaskDelay(1);
    }
    vTaskDelete( NULL );
}

void CONSOLE_TASK(void *pvParameters){
    UNUSED(pvParameters);
    char data[256] = {0};
    for (;;) {
        uint8_t rsize = xStreamBufferReceive(cli_stream, data, 255, portMAX_DELAY);
        for(uint8_t i = 0; i < rsize; i++){
            if(rl.last_index < 50){
                rl.last_index += 1;
            } else {
                rl.last_index = 0;
            }
            rl.buffer[rl.last_index] = data[i];

            if(rl.last_index != rl.current_index){
                if(rl.current_index < 50){
                    rl.current_index += 1;
                } else {
                    rl.current_index = 0;
                }
                microrl_insert_char(&rl, (int)(rl.buffer[rl.current_index]));
            }
        }
    }
    vTaskDelete( NULL );
}


#ifdef USE_GSM
void GSM_PRINT(void *pvParameters){
    UNUSED(pvParameters);
    char gsm_output[64] = {0};
    for (;;) {
        uint8_t rsize = xStreamBufferReceive(gsm_stream, gsm_output, 64, portMAX_DELAY);
        xSemaphoreTake(xSemaphore, 1000);
        for(uint8_t i = 0; i < rsize; i++){
            UART_tx(USART1, gsm_output[i]);
            if(gsm_output[i] == '\r'){
                if(sim7000g.rx_counter > 0)
                    sim7000g.rx_buf[sim7000g.rx_counter] = 0;
            }
            else if(gsm_output[i] == '\n'){
                if(sim7000g.rx_counter > 1)
                    GSM_AnswerParser();
            }
            else if(gsm_output[i] != 0){
                sim7000g.rx_buf[sim7000g.rx_counter] = gsm_output[i];
                sim7000g.rx_counter += 1;
            }
            if(strstr(sim7000g.rx_buf, "> ") != 0){
                sim7000g.status.tcp_ready_to_send = 1;
            }
        }
        xSemaphoreGive(xSemaphore);
        // if(strstr(sim7000g.rx_buf, "OK\r\n") == 0){
        //     GSM_AnswerParser();
        // }
    }
    vTaskDelete( NULL );
	// Delay(1000);
}
#endif
void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    xprintf("\nKERNEL PANIC! STACK OVERFLOW AT TASK %s\n", pcTaskName);
    UNUSED(xTask);
    while (1){};
}

void vApplicationIdleHook (void){
    IWDG_refresh();
}

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
    /* If the buffers to be provided to the Idle task are declared inside this
       function then they must be declared static - otherwise they will be allocated on
       the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
       state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
       Note that, as the array is necessarily of type StackType_t,
       configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

int main(){
    System_Init();

#if (USE_SEGGER == 1)
    SEGGER_SYSVIEW_Conf();
#endif

    if(system_config.action_mode){
        create_action_task();
    }
    xTaskCreateStatic( RX_RADIO_TASK, "RX_RADIO", RADIO_SIZE, NULL, 2, xStack_RX_RADIO, &xTaskBuffer_RX_RADIO);
    xTaskCreateStatic( TX_RADIO_TASK, "TX_RADIO", RADIO_SIZE, NULL, 2, xStack_TX_RADIO, &xTaskBuffer_TX_RADIO);
    // xTaskCreateStatic( GSM_PRINT, "GSM_PRINT", GSM_SIZE, NULL, 2, xStack_GSM_PRINT, &xTaskBuffer_RADIO_GSM_PRINT);
    xTaskCreateStatic( CONSOLE_TASK, "CONSOLE", CONSOLE_SIZE, NULL, 2, xStack_CONSOLE, &xTaskBuffer_RADIO_CONSOLE);
    vTaskStartScheduler();
}


