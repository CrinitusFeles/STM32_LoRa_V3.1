#include "main.h"
#include <stdarg.h>
#include "low_power.h"
#include "string.h"
#include "System.h"
#include "FreeRTOS.h"
#include "task.h"
#include "fifo.h"
#include "sx127x.h"
#include "monitor_task.h"
#include "xprintf.h"
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

FIFO fifo;
FIFO lora_fifo;
lua_State *L;
uint8_t answer_over_radio = 0;
uint8_t lora_tx_ptr = 0;
#define UNUSED(x) (void)(x)


void lora_set_tx_buff(uint8_t data){
    sx127x.tx_buffer[lora_tx_ptr] = data;
    lora_tx_ptr++;
    if(data == '\n' || lora_tx_ptr >= 127){
        LoRa_transmit(&sx127x, sx127x.tx_buffer, lora_tx_ptr);
        lora_tx_ptr = 0;
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


void LED_BLINK(void *pvParameters){
    UNUSED(pvParameters);
    for (;;) {
        gpio_toggle(LED);
        vTaskDelay(500);
    }
    vTaskDelete( NULL );
	// Delay(1000);
}

void RADIO_TASK(void *pvParameters){
    UNUSED(pvParameters);
    for (;;) {
        if(sx127x.got_new_packet){
            uint8_t rx_count = LoRa_receive(&sx127x, sx127x.rx_buffer, 255);
            for(uint8_t i = 0; i < rx_count; i++){
                FIFO_PUSH(fifo, sx127x.rx_buffer[i]);
            }
            // xprintf("\nGot msg: %s\n", sx127x.rx_buffer);
            answer_over_radio = 1;
            sx127x.got_new_packet = 0;
            xdev_out(lora_set_tx_buff);
        }
        vTaskDelay(15);
    }
    vTaskDelete( NULL );
}
void CONSOLE_TASK(void *pvParameters){
    UNUSED(pvParameters);
    for (;;) {
        if(!FIFO_IS_EMPTY(fifo)){
            if(rl.last_index < 50){
                rl.last_index += 1;
            } else {
                rl.last_index = 0;
            }
            rl.buffer[rl.last_index] = FIFO_FRONT(fifo);
            FIFO_POP(fifo);

            if(rl.last_index != rl.current_index){
                if(rl.current_index < 50){
                    rl.current_index += 1;
                } else {
                    rl.current_index = 0;
                }
                microrl_insert_char(&rl, (int)(rl.buffer[rl.current_index]));
            }
        }
        vTaskDelay(1);
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
    IWDG->KR = 0xAAAA;
}

static int lua_led_on(lua_State *L)
{
    gpio_state(LED, HIGH);
    // BUZZ_beep(&buzzer, 800, 100);
    printf("LED0->1\r\n");
    return 1;
}
static int lua_led_off(lua_State *L)
{
    // BUZZ_beep(&buzzer, 400, 100);
    gpio_state(LED, LOW);
    printf("LED0->0\r\n");
    return 1;
}

static int lua_delay(lua_State *L)
{
    int num;
    num= lua_tointeger(L, 1);
    DWT_Delay_ms(num);
    return 1;
}

static const luaL_Reg mylib[]=
{
  {"led_on",lua_led_on},
  {"led_off",lua_led_off},
  {"delay",lua_delay},
  {NULL,NULL}
};

const char LUA_SCRIPT_Test[] =
"  \
off = 500     \
on = 500       \
print(_VERSION)     \
led_on() \
delay(off)    \
led_off()        \
delay(on)      \
";
int main(){
    System_Init();
    // xTaskCreate( MonitorTask, "TRACE", configMINIMAL_STACK_SIZE, NULL, 1, ( xTaskHandle * ) NULL);
    // xTaskCreate( LED_BLINK, "LED1", configMINIMAL_STACK_SIZE, NULL, 2, ( xTaskHandle * ) NULL);
    // // xTaskCreate( PERIPH_TOGGLE, "PERIPH_TOGGLE", configMINIMAL_STACK_SIZE, NULL, 2, ( xTaskHandle * ) NULL);
    // xTaskCreate( CONSOLE_TASK, "CONSOLE", configMINIMAL_STACK_SIZE * 14, NULL, 2, ( xTaskHandle * ) NULL);
    // xTaskCreate( RADIO_TASK, "RADIO", configMINIMAL_STACK_SIZE * 2, NULL, 2, ( xTaskHandle * ) NULL);
    // vTaskStartScheduler();
//lua initialization part

	L=luaL_newstate();			//Create LUA state machine
	luaopen_base(L);
	luaL_setfuncs(L, mylib, 0);
    int status1, result1;
    lua_gc(L, LUA_GCSTOP);  /* stop GC while building state */
    lua_pushcfunction(L, &pmain);  /* to call 'pmain' in protected mode */
    // lua_pushinteger(L, argc);  /* 1st argument */
    // lua_pushlightuserdata(L, argv); /* 2nd argument */
    status1 = lua_pcall(L, 2, 1, 0);  /* do the call */
    result1 = lua_toboolean(L, -1);  /* get result */
    // report(L, status);
    // lua_close(L);
    while (1)
    {
        luaL_dostring(L,LUA_SCRIPT_Test);		//Run LUA script
    }
}


