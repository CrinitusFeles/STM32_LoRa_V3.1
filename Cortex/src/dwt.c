#include "dwt.h"

uint32_t DWT_GetDelta(uint32_t t0) {
    return DWT->CYCCNT - t0;
}

uint32_t DWT_GetMicro() {
    return DWT->CYCCNT / 80;
}

uint32_t DWT_Get_Current_Tick() {
    return DWT->CYCCNT;
}


uint32_t DWT_Elapsed_Tick(uint32_t t0) {
    if (DWT->CYCCNT > t0)
        return DWT->CYCCNT - t0;

    return (uint32_t) ((((uint64_t) 0x100000000) + DWT->CYCCNT) - t0);
}


void DWT_Init() {
    if (!(CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk)) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}


void DWT_Delay_us(uint32_t us) { // microseconds
    uint32_t t0 = DWT->CYCCNT;
    uint32_t delta = us * DWT_IN_MICROSEC;

    while (DWT_GetDelta(t0) < delta) {}
}


inline void DWT_Delay_ms(uint32_t ms) { // milliseconds
    DWT_Delay_us(ms * 1000);
}


void DWT_Delay_With_Action(uint32_t us, int (*cond)(), void (*act)()) { // microseconds
    uint32_t t0 = DWT->CYCCNT;
    uint32_t delta = us * DWT_IN_MICROSEC;
    static uint32_t trigger = 0;

    while (DWT_GetDelta(t0) < delta) {
        if (trigger == 1) {
            if (cond() == 0)
                trigger = 0;
        } else {
            if (cond() == 1) {
                trigger = 1;
                act();
            }
        }
    }
}

