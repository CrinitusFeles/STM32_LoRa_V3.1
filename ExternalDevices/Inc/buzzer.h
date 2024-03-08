
#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include "stm32l431xx.h"

typedef struct sBuzzer{
    TIM_TypeDef *TIMx;
    int channel;
    void(*delay)(const uint32_t);
} tBuzzer;
void BUZZ_tmnt(tBuzzer *this);
void BUZZ_underground(tBuzzer *this);
void BUZZ_mario(tBuzzer *this);
void BUZZ_down(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count);
void BUZZ_up(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count);
void BUZZ_beep_repeat(tBuzzer* this, uint16_t freq, uint16_t duration_ms, uint16_t period, uint16_t count);
void BUZZ_beep(tBuzzer* this, uint16_t freq, uint16_t duration_ms);
extern tBuzzer buzzer;
// void Buzzer_down(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count);
// void Buzzer_up(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count);
// void Buzzer_beep_repeat(tBuzzer* this, uint16_t freq, uint16_t duration_ms, uint16_t period, uint16_t count);
// void Buzzer_beep(tBuzzer* this, uint16_t freq, uint16_t duration_ms);

#endif  //INC_BUZZER_H_