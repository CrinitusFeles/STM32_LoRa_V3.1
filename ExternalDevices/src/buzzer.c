#include "buzzer.h"
#include "pwm.h"
#include "gpio.h"


void BUZZ_beep(tBuzzer* this, uint16_t freq, uint16_t duration_ms){
    PWM_init(this->TIMx, this->channel, freq, 50);
    PWM_start_single(this->TIMx, this->channel);
    this->delay(duration_ms);
    PWM_stop_single(this->TIMx, this->channel);
}

void BUZZ_beep_repeat(tBuzzer* this, uint16_t freq, uint16_t duration_ms, uint16_t period, uint16_t count){
    for(uint16_t i = 0; i < count; i++){
        BUZZ_beep(this, freq, duration_ms);
        this->delay(period);
    }
}

void BUZZ_up(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count){
    for(uint32_t j = 0; j < count; j ++){
        for(uint32_t freq = start_freq; freq < end_freq; freq += step){
            BUZZ_beep(this, freq, step_time_ms);
        }
    }
}

void BUZZ_down(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count){
    for(uint32_t j = 0; j < count; j ++){
        for(uint32_t freq = start_freq; freq > end_freq; freq -= step){
            BUZZ_beep(this, freq, step_time_ms);
        }
    }
}

void BUZZ_mario(tBuzzer *this){
    BUZZ_beep(this, 660, 200);
    this->delay(100);
    BUZZ_beep(this, 660, 100);
    this->delay(100);
    BUZZ_beep(this, 660, 100);
    this->delay(100);
    BUZZ_beep(this, 523, 100);
    this->delay(100);
    BUZZ_beep(this, 660, 200);
    this->delay(50);
    BUZZ_beep(this, 784, 400);
    this->delay(200);
    BUZZ_beep(this, 392, 100);
}

void BUZZ_underground(tBuzzer *this){
    for(uint8_t i = 0; i < 2; i++){
        BUZZ_beep(this, 130, 170);
        BUZZ_beep(this, 523, 170);

        BUZZ_beep(this, 110, 170);
        BUZZ_beep(this, 440, 170);

        BUZZ_beep(this, 233, 170);
        BUZZ_beep(this, 466, 170);
        this->delay(1000);
    }
    for(uint8_t i = 0; i < 2; i++){
        BUZZ_beep(this, 87, 170);
        BUZZ_beep(this, 174, 170);

        BUZZ_beep(this, 73, 170);
        BUZZ_beep(this, 147, 170);

        BUZZ_beep(this, 78, 170);
        BUZZ_beep(this, 155, 170);
        this->delay(1000);
    }
}

void BUZZ_tmnt(tBuzzer *this){
    BUZZ_beep(this, 261, 200);
    BUZZ_beep(this, 440, 200);
    BUZZ_beep(this, 392, 200);
    BUZZ_beep(this, 440, 200);

    BUZZ_beep(this, 261, 200);
    BUZZ_beep(this, 440, 100);
    BUZZ_beep(this, 392, 300);
    BUZZ_beep(this, 440, 200);
    this->delay(30);

    BUZZ_beep(this, 311, 200);
    BUZZ_beep(this, 523, 200);
    BUZZ_beep(this, 466, 200);
    BUZZ_beep(this, 523, 200);

    BUZZ_beep(this, 311, 200);
    BUZZ_beep(this, 523, 100);
    BUZZ_beep(this, 466, 300);
    BUZZ_beep(this, 523, 200);

    BUZZ_beep(this, 349, 200);
    BUZZ_beep(this, 523, 200);
    BUZZ_beep(this, 466, 200);
    BUZZ_beep(this, 523, 200);

    BUZZ_beep(this, 622, 200);
    BUZZ_beep(this, 523, 100);
    BUZZ_beep(this, 466, 300);
    BUZZ_beep(this, 523, 200);

    for(uint8_t i = 0; i < 4; i++){
        BUZZ_beep(this, 261, 100);
        this->delay(30);
    }
    BUZZ_beep(this, 233, 200);
    BUZZ_beep(this, 261, 200);

    this->delay(200);
    for(uint8_t i = 0; i < 4; i++){
        BUZZ_beep(this, 261, 100);
        this->delay(30);
    }
}



