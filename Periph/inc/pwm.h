#ifndef CODE_INC_PWM_H_
#define CODE_INC_PWM_H_
#include "stm32l431xx.h"


enum pwm_channels {
	PWM_CH1=1, PWM_CH2, PWM_CH3, PWM_CH4
};
void PWM_init(TIM_TypeDef *timY, int channel, int freq, int fill);
void PWM_start_single(TIM_TypeDef *timer, int channel);
void PWM_change_freq(TIM_TypeDef *TIMx, uint16_t freq);
void PWM_change_fill(TIM_TypeDef *timer, int channel, int fill);
void PWM_stop_single(TIM_TypeDef *timer, int channel);
void PWM_deinit(TIM_TypeDef *timer);
void PWM_start_all(TIM_TypeDef *timer);
void PWM_stop_all(TIM_TypeDef *timer);


#endif /* CODE_INC_PWM_H_ */
