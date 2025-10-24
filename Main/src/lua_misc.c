#include "FreeRTOS.h"
#include "task.h"
#include "lua_misc.h"
#include "time.h"
#include "xprintf.h"


#define UNUSED(x) (void)(x)

time_t time(time_t *time)
{
    UNUSED(time);
    return 0;
}
#ifdef USE_LUA
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include "ltable.h"
#include "lstring.h"


StackType_t xStack_LUA [configMINIMAL_STACK_SIZE * 16];
StaticTask_t xTaskBuffer_LUA;
TaskHandle_t lua_task;
lua_State *L;


static int lua_led_on(lua_State *L)
{
    UNUSED(L);
    // gpio_state(LED, HIGH);
    // BUZZ_beep(&buzzer, 800, 100);
    printf("LED0->1\r\n");
    return 1;
}
static int lua_led_off(lua_State *L)
{
    UNUSED(L);
    // BUZZ_beep(&buzzer, 400, 100);
    // gpio_state(LED, LOW);
    printf("LED0->0\r\n");
    return 1;
}

static int lua_delay(lua_State *L)
{
    int num= lua_tointeger(L, 1);
    vTaskDelay(num);
    return 1;
}

static const luaL_Reg mylib[]=
{
  {"led_on",lua_led_on},
  {"led_off",lua_led_off},
  {"delay",lua_delay},
  {NULL,NULL}
};


void LUA_TASK(void *pvParameters){
	L=luaL_newstate();			//Create LUA state machine
	luaopen_base(L);
    // luaopen_table(L);
    // luaopen_string(L);
    lua_gc(L, LUA_GCSTOP);  /* stop GC while building state */
	luaL_setfuncs(L, mylib, 0);
    if(luaL_loadstring(L, (char*)pvParameters)){
        xprintf("\n\nFailed to run script %s\n", lua_tostring(L,-1));
    }
    // luaL_dostring
    xprintf("\n\r");
    if(lua_pcall(L, 0, 0, 0)){
        xprintf("Running error: %s\n", lua_tostring(L,-1));
    }
    vTaskDelete( NULL );
}

void create_lua_task(char *lua_code){
    lua_task = xTaskCreateStatic(
        LUA_TASK, "LUA", configMINIMAL_STACK_SIZE * 16,
        (void*)lua_code, 0, xStack_LUA, &xTaskBuffer_LUA
    );
}
#endif