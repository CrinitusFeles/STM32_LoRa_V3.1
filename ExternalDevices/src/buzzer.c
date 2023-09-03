#include "delay.h"
#include "buzzer.h"
#include "pwm.h"
#include "gpio.h"


void beep(tBuzzer* this, uint16_t freq, uint16_t duration_ms){
    PWM_init(this->__TIMx, this->__channel, freq, 50);
    PWM_start_single(this->__TIMx, this->__channel);
    Delay(duration_ms);
    PWM_stop_single(this->__TIMx, this->__channel);
}

void beep_repeat(tBuzzer* this, uint16_t freq, uint16_t duration_ms, uint16_t period, uint16_t count){
    for(uint16_t i = 0; i < count; i++){
        beep(this, freq, duration_ms);
        Delay(period);
    }
}

void up(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count){
    for(uint32_t j = 0; j < count; j ++){
        for(uint32_t freq = start_freq; freq < end_freq; freq += step){
            beep(this, freq, step_time_ms);
        }
    }
}

void down(tBuzzer* this, uint16_t start_freq, uint16_t end_freq, uint16_t step, uint16_t step_time_ms, uint16_t count){
    for(uint32_t j = 0; j < count; j ++){
        for(uint32_t freq = start_freq; freq > end_freq; freq -= step){
            beep(this, freq, step_time_ms);
        }
    }
}

void mario(tBuzzer *this){
    beep(this, 660, 200);
    Delay(100);
    beep(this, 660, 100);
    Delay(100);
    beep(this, 660, 100);
    Delay(100);
    beep(this, 523, 100);
    Delay(100);
    beep(this, 660, 200);
    Delay(50);
    beep(this, 784, 400);
    Delay(200);
    beep(this, 392, 100);
}

void underground(tBuzzer *this){
    for(uint8_t i = 0; i < 2; i++){
        beep(this, 130, 170);
        beep(this, 523, 170);

        beep(this, 110, 170);
        beep(this, 440, 170);

        beep(this, 233, 170);
        beep(this, 466, 170);
        Delay(1000);
    }
    for(uint8_t i = 0; i < 2; i++){
        beep(this, 87, 170);
        beep(this, 174, 170);

        beep(this, 73, 170);
        beep(this, 147, 170);

        beep(this, 78, 170);
        beep(this, 155, 170);
        Delay(1000);
    }
}

void tmnt(tBuzzer *this){
    beep(this, 261, 200);
    beep(this, 440, 200);
    beep(this, 392, 200);
    beep(this, 440, 200);

    beep(this, 261, 200);
    beep(this, 440, 100);
    beep(this, 392, 300);
    beep(this, 440, 200);
    Delay(30);

    beep(this, 311, 200);
    beep(this, 523, 200);
    beep(this, 466, 200);
    beep(this, 523, 200);

    beep(this, 311, 200);
    beep(this, 523, 100);
    beep(this, 466, 300);
    beep(this, 523, 200);

    beep(this, 349, 200);
    beep(this, 523, 200);
    beep(this, 466, 200);
    beep(this, 523, 200);

    beep(this, 622, 200);
    beep(this, 523, 100);
    beep(this, 466, 300);
    beep(this, 523, 200);

    for(uint8_t i = 0; i < 4; i++){
        beep(this, 261, 100);
        Delay(30);
    }
    beep(this, 233, 200);
    beep(this, 261, 200);

    Delay(200);
    for(uint8_t i = 0; i < 4; i++){
        beep(this, 261, 100);
        Delay(30);
    }
}


tBuzzer Buzzer(TIM_TypeDef *TIMx, int channel, int pin, int pin_func){
    tBuzzer this;
    gpio_init(pin, pin_func, Push_pull, no_pull, Very_high_speed);
    this.__channel = channel;
    this.__pin = pin;
    this.__pin_func = pin_func;
    this.__TIMx = TIMx;

    this.beep = beep;
    this.beep_repeat = beep_repeat;
    this.up = up;
    this.down = down;
    this.mario = mario;
    this.mario_underground = underground;
    this.tmnt = tmnt;

    return this;
}



