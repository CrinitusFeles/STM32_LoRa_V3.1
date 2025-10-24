#include "main.h"
#include <stdarg.h>
#include "low_power.h"
#include "string.h"
#include "System.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sx127x.h"
// #include "sx126x.h"
#include "monitor_task.h"
#include "xprintf.h"
#include "iwdg.h"
#include "stm32_misc.h"
#include "gsm.h"
#include "periph_handlers.h"
#include "lua_misc.h"

#define UNUSED(x) (void)(x)


uint8_t timeout_counter = 0;
uint8_t self_addr = 31;
StreamBufferHandle_t  cli_stream;
StreamBufferHandle_t  gsm_stream;
SemaphoreHandle_t xSemaphore;
StaticTask_t xTaskBuffer_RADIO;
StaticTask_t xTaskBuffer_RADIO_GSM_PRINT;
StaticTask_t xTaskBuffer_RADIO_CONSOLE;
StackType_t xStack_RADIO [configMINIMAL_STACK_SIZE];
StackType_t xStack_GSM_PRINT [configMINIMAL_STACK_SIZE];
StackType_t xStack_CONSOLE [configMINIMAL_STACK_SIZE * 12];


static const uint16_t crc16_ccitt_table_reverse[256] = {
   0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
   0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
   0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
   0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
   0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
   0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
   0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
   0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
   0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
   0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
   0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
   0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
   0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
   0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
   0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
   0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
   0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
   0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
   0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
   0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
   0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
   0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
   0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
   0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
   0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
   0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
   0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
   0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
   0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
   0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
   0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
   0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78
};

uint16_t crc16_calc(uint8_t *buffer, uint16_t len){
   uint16_t fcs = 0xFFFF;
   while (len--)
      fcs = (fcs >> 8) ^ crc16_ccitt_table_reverse[(fcs ^ *buffer++) & 0xFF];
   return fcs ^ 0xFFFF;  // bit reversed
}


void route_cli_to_lora(uint8_t data){
    while(sx127x.transmitting_progress)
        vTaskDelay(1);
    sx127x.tx_data.payload[sx127x.tx_data.dlen] = data;
    timeout_counter = 0;
    sx127x.tx_data.dlen++;
    if(sx127x.tx_data.dlen == 250){
        sx127x.transmitting_progress = 1;
        return;
    }
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

void RADIO_TASK(void *pvParameters){
    UNUSED(pvParameters);
    for (;;) {
        if(sx127x.new_rx_data_flag){
            memset(sx127x.rx_data.payload, 0, 250);
            uint8_t cnt = LoRa_receive(&sx127x, sx127x.rx_data.buffer, 255);
            sx127x.new_rx_data_flag = 0;
            if(sx127x.rx_data.dst_addr != self_addr)
                continue;
            uint16_t crc16 = crc16_calc(sx127x.rx_data.payload, sx127x.rx_data.dlen);
            if(crc16 != sx127x.rx_data.crc16)
                continue;

            xdev_out(route_cli_to_lora);
            for(uint8_t i = 0; i < sx127x.rx_data.dlen; i++){
                xStreamBufferSend(cli_stream, &sx127x.rx_data.payload[i], cnt, portMAX_DELAY);
            }
            xStreamBufferSend(cli_stream, "\n\r", 3, portMAX_DELAY);
        }
        if(sx127x.tx_data.dlen > 1){
            timeout_counter += 1;
        }
        if((sx127x.tx_data.dlen == 250) || timeout_counter >= 10){
            sx127x.tx_data.crc16 = crc16_calc(sx127x.tx_data.payload, sx127x.tx_data.dlen);
            sx127x.tx_data.src_addr = self_addr;
            sx127x.tx_data.dst_addr = sx127x.rx_data.src_addr;
            LoRa_transmit(&sx127x, sx127x.tx_data.buffer, sx127x.tx_data.dlen + 5);
            timeout_counter = 0;
        }
        vTaskDelay(15);
    }
    vTaskDelete( NULL );
}

void CONSOLE_TASK(void *pvParameters){
    UNUSED(pvParameters);
    char data[256] = {0};
    for (;;) {
        uint8_t rsize = xStreamBufferReceive(cli_stream, data, 256, portMAX_DELAY);
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
	// Delay(1000);
}



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
        }
        xSemaphoreGive(xSemaphore);
        // if(strstr(sim7000g.rx_buf, "OK\r\n") == 0){
        //     GSM_AnswerParser();
        // }
    }
    vTaskDelete( NULL );
	// Delay(1000);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName)
{
    UNUSED(xTask);
    xprintf("\nKERNEL PANIC! STACK OVERFLOW AT TASK %s\n", pcTaskName);
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
    // xTaskCreate( MonitorTask, "TRACE", configMINIMAL_STACK_SIZE, NULL, 1, ( xTaskHandle * ) NULL);
    // xTaskCreate( PERIPH_TOGGLE, "PERIPH_TOGGLE", configMINIMAL_STACK_SIZE, NULL, 2, ( xTaskHandle * ) NULL);
    xTaskCreateStatic( RADIO_TASK, "RADIO", configMINIMAL_STACK_SIZE, NULL, 2, xStack_RADIO, &xTaskBuffer_RADIO);
    xTaskCreateStatic( GSM_PRINT, "GSM_PRINT", configMINIMAL_STACK_SIZE, NULL, 2, xStack_GSM_PRINT, &xTaskBuffer_RADIO_GSM_PRINT);
    xTaskCreateStatic( CONSOLE_TASK, "CONSOLE", configMINIMAL_STACK_SIZE * 12, NULL, 2, xStack_CONSOLE, &xTaskBuffer_RADIO_CONSOLE);

    vTaskStartScheduler();
}


