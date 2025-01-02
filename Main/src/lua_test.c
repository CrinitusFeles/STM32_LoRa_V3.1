// #include "lua.h"
// #include "lauxlib.h"
// #include "lualib.h"


// lua_State *L;

// static int lua_led_on(lua_State *L)
// {
//     gpio_state(LED, HIGH);
//     // BUZZ_beep(&buzzer, 800, 100);
//     printf("LED0->1\r\n");
//     return 1;
// }
// static int lua_led_off(lua_State *L)
// {
//     // BUZZ_beep(&buzzer, 400, 100);
//     gpio_state(LED, LOW);
//     printf("LED0->0\r\n");
//     return 1;
// }

// static int lua_delay(lua_State *L)
// {
//     int num;
//     num= lua_tointeger(L, 1);
//     DWT_Delay_ms(num);
//     return 1;
// }

// static const luaL_Reg mylib[]=
// {
//   {"led_on",lua_led_on},
//   {"led_off",lua_led_off},
//   {"delay",lua_delay},
//   {NULL,NULL}
// };

// const char LUA_SCRIPT_Test[] =
// "  \
// off = 500     \
// on = 500       \
// print(_VERSION)     \
// led_on() \
// delay(off)    \
// led_off()        \
// delay(on)      \
// ";

// int main(){
//     System_Init();
//     // xTaskCreate( MonitorTask, "TRACE", configMINIMAL_STACK_SIZE, NULL, 1, ( xTaskHandle * ) NULL);
//     // xTaskCreate( LED_BLINK, "LED1", configMINIMAL_STACK_SIZE, NULL, 2, ( xTaskHandle * ) NULL);
//     // // xTaskCreate( PERIPH_TOGGLE, "PERIPH_TOGGLE", configMINIMAL_STACK_SIZE, NULL, 2, ( xTaskHandle * ) NULL);
//     // xTaskCreate( CONSOLE_TASK, "CONSOLE", configMINIMAL_STACK_SIZE * 14, NULL, 2, ( xTaskHandle * ) NULL);
//     // xTaskCreate( RADIO_TASK, "RADIO", configMINIMAL_STACK_SIZE * 2, NULL, 2, ( xTaskHandle * ) NULL);
//     // vTaskStartScheduler();
// //lua initialization part

// 	L=luaL_newstate();			//Create LUA state machine
// 	luaopen_base(L);
// 	luaL_setfuncs(L, mylib, 0);
//     int status1, result1;
//     lua_gc(L, LUA_GCSTOP);  /* stop GC while building state */
//     lua_pushcfunction(L, &pmain);  /* to call 'pmain' in protected mode */
//     // lua_pushinteger(L, argc);  /* 1st argument */
//     // lua_pushlightuserdata(L, argv); /* 2nd argument */
//     status1 = lua_pcall(L, 2, 1, 0);  /* do the call */
//     result1 = lua_toboolean(L, -1);  /* get result */
//     // report(L, status);
//     // lua_close(L);
//     while (1)
//     {
//         luaL_dostring(L,LUA_SCRIPT_Test);		//Run LUA script
//     }
// }