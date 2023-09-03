
#ifndef INC_BUZZER_H_
#define INC_BUZZER_H_

#include "stm32l431xx.h"

typedef struct sBuzzer{
    TIM_TypeDef *__TIMx;
    int __channel;
    int __pin;
    int __pin_func;

    void (*beep)(struct sBuzzer* this, uint16_t freq, uint16_t duration_ms);
    void (*beep_repeat)(struct sBuzzer* this, uint16_t freq, uint16_t duration_ms, uint16_t period, uint16_t count);
    void (*up)(struct sBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count);
    void (*down)(struct sBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count);
    void (*mario)(struct sBuzzer* this);
    void (*mario_underground)(struct sBuzzer* this);
    void (*tmnt)(struct sBuzzer* this);
} tBuzzer;

tBuzzer Buzzer(TIM_TypeDef *TIMx, int channel, int pin, int pin_func);
// void Buzzer_down(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count);
// void Buzzer_up(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count);
// void Buzzer_beep_repeat(tBuzzer* this, uint16_t freq, uint16_t duration_ms, uint16_t period, uint16_t count);
// void Buzzer_beep(tBuzzer* this, uint16_t freq, uint16_t duration_ms);

#endif  //INC_BUZZER_H_