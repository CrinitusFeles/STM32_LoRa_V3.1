#include "pwm.h"
#include "System.h"


#define TIMER_DEVIDER 2 //TIMx->PSC

int global_freq; //глабальная переменная для значения частоты, передаваемой в функцию инициализации. Данное значение надо как-то передать функции
				 // ChangeFilling(TIM_TypeDef *TIMx , int channel, int fill) т.к. в ней надо рассчитать процент заполнения ШИМа.

void PWM_init(TIM_TypeDef *TIMx, int channel, int freq, int fill){
	/*
	 * Функция инициализации канала таймера.
	 * TIM_TypeDef *TIMx - регистр таймера, поддерживающего ШИМ режим, например TIM1, TIM3 и т.д.
	 * int channel - номер канала таймера. Принимает на вход цифру 1-4 или значение из нумерованного списка PWM_CH1-PWM_CH4.
	 * int freq - частота ШИМа. Ограничено значением TIMER_DEVIDER. Чем больше значение TIMER_DEVIDER, тем на меньшей частоте может работать шим.
	 * 			  для TIMER_DEVIDER=1 и частоты тактирования таймера = 72МГц минимальная частота ШИМ = 550Гц.
	 * 			  Общая формула для вычисления частоты ШИМ PWM_freq = timer_clk / (TIMx->ARR * (TIMx->PSC + 1))
	 * int fill - процент заполнения ШИМа. Может принимать значения 0-100.
	 */

	global_freq = freq;

	if(TIMx == TIM1) {
		RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
//		if(channel == 1) 	  gpio_init(PA8, Alternative_PP, Max_50_MHz);
//		else if(channel == 2) gpio_init(PA9, Alternative_PP, Max_50_MHz);
//		else if(channel == 3) gpio_init(PA10, Alternative_PP, Max_50_MHz);
//		else if(channel == 4) gpio_init(PA11, Alternative_PP, Max_50_MHz);
	}

//		if(channel == PWM_CH1) 	  gpio_init(PA6, Alternative_PP, Max_50_MHz);
//		else if(channel == PWM_CH2) gpio_init(PA7, Alternative_PP, Max_50_MHz);
//		else if(channel == PWM_CH3) gpio_init(PB0, Alternative_PP, Max_50_MHz);
//		else if(channel == PWM_CH4) gpio_init(PB1, Alternative_PP, Max_50_MHz);

	else if(TIMx == TIM15){
		RCC->APB2ENR |= RCC_APB2ENR_TIM15EN;
	}
	else if(TIMx == TIM2){
		RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	}
	TIMx->PSC = TIMER_DEVIDER; // !ЗАМЕТКА! (При увеличении значения делителя сильно падает точность)

	int pwm_freq = F_CPU / ((TIMER_DEVIDER + 1) * freq); //для TIMx->PSC = 1 минимальная частота 550Гц (т.к. TIMx->ARR 16 бит)
	TIMx->ARR = pwm_freq; //для TIMx->PSC = 1 минимальная частота 550Гц (т.к. TIMx->ARR 16 бит)

	switch (channel) {  //включаем выбранный канал
		case PWM_CH1:
			TIMx->CCR1 = pwm_freq * fill / 100; 	//процент заполнения
			TIMx->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_0;		//PWM mode 1, инверсный ШИМ 1 канал
			break;
		case PWM_CH2:
			TIMx->CCR2 = pwm_freq * fill / 100;	//процент заполнения
			TIMx->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_0;		//PWM mode 1, инверсный ШИМ 2 канал
			break;
		case PWM_CH3:
			TIMx->CCR3 = pwm_freq * fill / 100;	//процент заполнения
			TIMx->CCMR2 |= TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_0;	 	//PWM mode 1, инверсный ШИМ 3 канал
			break;
		case PWM_CH4:
			TIMx->CCR4 = pwm_freq * fill / 100;	//процент заполнения
			TIMx->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_0;		//PWM mode 1, инверсный ШИМ 4 канал
			break;
		default:
			break;
	}
	// TIMx->CCER |= TIM_CCER_CC4E | TIM_CCER_CC4P | TIM_CCER_CC3E | TIM_CCER_CC3P | TIM_CCER_CC2E | TIM_CCER_CC2P | TIM_CCER_CC1E | TIM_CCER_CC1P;
	TIMx->BDTR |= TIM_BDTR_MOE; 	//активирует пины как выходы при настроенном регистре TIMx_CCER. странно... работает и без этого...
	TIMx->CR1 &= ~TIM_CR1_DIR; 	//считаем вверх
	TIMx->CR1 &= ~TIM_CR1_CMS; 	//выравнивание по фронту, Fast PWM
	TIMx->CR1 |= TIM_CR1_CEN;		//запускаем таймер
}

void PWM_change_freq(TIM_TypeDef *TIMx, uint16_t freq){
    global_freq = freq;
    int pwm_freq = F_CPU / ((TIMER_DEVIDER + 1) * freq); //для TIMx->PSC = 1 минимальная частота 550Гц (т.к. TIMx->ARR 16 бит)
	TIMx->ARR = pwm_freq; //для TIMx->PSC = 1 минимальная частота 550Гц (т.к. TIMx->ARR 16 бит)
}

void PWM_start_single(TIM_TypeDef *TIMx, int channel){
	TIMx->CCER |= (0x3 << (4 * (channel-1))); //пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅ пїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅ.
}

void PWM_start_all(TIM_TypeDef *TIMx ){
	//настроим на выход канал.
	TIMx->CCER |= TIM_CCER_CC4E | TIM_CCER_CC4P | TIM_CCER_CC3E | TIM_CCER_CC3P; //| TIM_CCER_CC2E | TIM_CCER_CC2P | TIM_CCER_CC1E | TIM_CCER_CC1P;
}

void PWM_stop_all(TIM_TypeDef *TIMx ){
	//функция для остановки всех каналов одновременно
	TIMx->CCER &= ~(TIM_CCER_CC4E | TIM_CCER_CC4P | TIM_CCER_CC3E | TIM_CCER_CC3P); //| TIM_CCER_CC2E | TIM_CCER_CC2P | TIM_CCER_CC1E | TIM_CCER_CC1P);
}

void PWM_change_fill(TIM_TypeDef *TIMx , int channel, int fill){
	//изменяет заполнения ШИМа на выбранном канале.
	/*
	 * TIM_TypeDef *TIMx - регистр таймера, поддерживающего ШИМ режим, например TIM1, TIM3 и т.д.
	 * int channel - номер канала таймера. Принимает на вход цифру 1-4 или значение из нумерованного списка PWM_CH1-PWM_CH4.
	 * int fill - процент заполнения ШИМа. Может принимать значения 0-100.
	 */

	if(channel == PWM_CH1) TIMx->CCR1 = F_CPU / ((TIMER_DEVIDER + 1) * global_freq) * fill / 100; // F_CPU / ((TIMER_DEVIDER + 1) * global_freq) - пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅ
	else if(channel == PWM_CH2) TIMx->CCR2 = F_CPU / ((TIMER_DEVIDER + 1) * global_freq) * fill / 100; // PWM_freq * fill /100 - пїЅпїЅпїЅпїЅпїЅпїЅпїЅ пїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅпїЅ
	else if(channel == PWM_CH3) TIMx->CCR3 = F_CPU / ((TIMER_DEVIDER + 1) * global_freq) * fill / 100;
	else if(channel == PWM_CH4) TIMx->CCR4 = F_CPU / ((TIMER_DEVIDER + 1) * global_freq) * fill / 100;
}

void PWM_stop_single(TIM_TypeDef *TIMx , int channel){
	/*
	 * Функция отключения одного указанного канала.
	 */
	TIMx->CCER &= ~(0x3 << (4 * (channel-1)));
//	TIMx->BDTR &= ~TIM_BDTR_MOE;
//	TIMx->CR1 &= ~TIM_CR1_CEN;

}

void PWM_deinit(TIM_TypeDef *TIMx ){
	/*
	 * Функция деинициализации указанного таймера, т.е. отключает ШИМ, переводит регистры таймера и пины мк в начальное состояние
	 * TIM_TypeDef *TIMx - регистр таймера, поддерживающего ШИМ режим, например TIM1, TIM3 и т.д.
	 */
	TIMx->CCMR2 = 0;
	TIMx->CCMR1 = 0;
	TIMx->CCR1 = 0;
	TIMx->CCR2 = 0;
	TIMx->CCR3 = 0;
	TIMx->CCR4 = 0;
	TIMx->CCER = 0;
	TIMx->ARR = 0;
	if(TIMx == TIM1) {
		RCC->APB2ENR &= ~RCC_APB2ENR_TIM1EN;
//		gpio_init(PA8, General_PP, Max_2_MHz);
//		gpio_init(PA9, General_PP, Max_2_MHz);
//		gpio_init(PA10, General_PP, Max_2_MHz);
//		gpio_init(PA11, General_PP, Max_2_MHz);
	}

}
