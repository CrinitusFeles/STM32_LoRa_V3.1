#include "tim.h"

void TIM_init(TIM_TypeDef *TIMx, uint32_t delay, uint8_t loop_mode){

    if(TIMx == TIM2){
        RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
    }
    else if(TIMx == TIM15){
    	RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
    }
    else if(TIMx == TIM16){
    	RCC->APB2ENR |= RCC_APB2ENR_TIM16EN;
    }
    else if(TIMx == TIM7){
    	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM7EN;
    }

    // (PSC + 1) * ARR = (AHB_freq / 1000) (for 1 milli)    (AHB_freq = 80MHz)
    TIMx->PSC = 39999;
    TIMx->ARR = 2 * delay;

    if(!loop_mode){
    	TIMx->CR1 |= TIM_CR1_OPM;
        TIMx->EGR |= TIM_EGR_UG;
        TIMx->SR &= ~TIM_SR_UIF;
    }

    TIMx->DIER |= TIM_DIER_UIE;

    if(TIMx == TIM2){
        NVIC_EnableIRQ(TIM2_IRQn);
    }
    else if(TIMx == TIM7){
        NVIC_EnableIRQ(TIM7_IRQn);
    }
    TIMx->CR1 |= TIM_CR1_CEN;

    if(TIMx == TIM15 || TIMx == TIM16){
        for(uint16_t i = 0; i < 30000; i++);
    }
}
