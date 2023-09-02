#include "gpio.h"

/*
 * 	 _______________________________________________________
 * 	| MODE | OTYPER | OSPEED | PUPD  |	I/O configuration	|
 * 	|______|________|________|_______|______________________|
 * 	|	   |	0	|		 | 0 | 0 | GP output | PP		|
 * 	|	   |	0	|		 | 0 | 1 | GP output | PP + PU	|
 * 	|	   |	0	|	00	 | 1 | 0 | GP output | PP + PD	|
 * 	|	   |	0	|	01	 | 1 | 1 | Reserved				|
 * 	|  01  |	1	|	10	 | 0 | 0 | GP output | OD		|
 * 	|	   |	1	|	11	 | 0 | 1 | GP output | OD + PU	|
 * 	|	   |	1	|		 | 1 | 0 | GP output | OD + PD	|
 * 	|______|____1___|________|_1_|_1_|_Reserved_____________|
 * 	|	   |	0	|		 | 0 | 0 | AF 		 | PP		|
 * 	|	   |	0	|		 | 0 | 1 | AF 		 | PP + PU	|
 * 	|	   |	0	|	00	 | 1 | 0 | AF 		 | PP + PD	|
 * 	|	   |	0	|	01	 | 1 | 1 | Reserved				|
 * 	|  10  |	1	|	10	 | 0 | 0 | AF 		 | OD		|
 * 	|	   |	1	|	11	 | 0 | 1 | AF 		 | OD + PU	|
 * 	|	   |	1	|		 | 1 | 0 | AF 		 | OD + PD	|
 * 	|______|____1___|________|_1_|_1_|_Reserved_____________|
 * 	|	   |	x	| x  | x | 0 | 0 | Input	 | Floating |
 * 	|	   |	x	| x  | x | 0 | 1 | Input	 | PU		|
 * 	|  00  |	x	| x  | x | 1 | 0 |_Input_____|_PD_______|
 * 	|	   |	x	| x  | x | 1 | 1 |       Reserved		|
 * 	|______|________|____|___|___|___|______________________|
 * 	|	   |	x	| x  | x | 0 | 0 |_In/Out____|_Analog___|
 * 	|	   |	x	| x  | x | 0 | 0 | 						|
 * 	|  11  |	x	| x  | x | 0 | 0 | 		Reserved		|
 * 	|	   |	x	| x  | x | 0 | 0 | 						|
 * 	|______|________|____|___|___|___|______________________|
 *
 */
#define NUMBER_OF_PINS_ON_EACH_PORT 	16

enum GPIO_Port {
	Port_A, Port_B, Port_C, Port_D, Port_E, Port_F, Port_G, Port_H
};
typedef struct GPIO_Struct{
    GPIO_TypeDef *port;
    uint8_t pin;
    uint8_t port_num;
} GPIO_Struct;

GPIO_Struct gpio_calculate(GPIO_Pin gpio){
    if(gpio == uninitialized){
        // while(1) NVIC_SystemReset();
        while(1);
    }
    gpio--;
    static GPIO_Struct temp = {0};
	temp.port_num = gpio / NUMBER_OF_PINS_ON_EACH_PORT;  // GPIOA = 1, GPIOB = 2, ..., GPIOE = 5
	temp.pin = gpio % NUMBER_OF_PINS_ON_EACH_PORT;  // PB0 = 0, PA1 = 1, PD4 = 4
    temp.port = (GPIO_TypeDef *)(AHB2PERIPH_BASE + (0x0400UL * temp.port_num) );
    return temp;
}

/// @brief Инициализация пина
/// @param gpio номер пина (PA1, PB12, PC0, ...)
/// @param mode режим работы (Input_mode, General_output, Analog_mode, or Alternatuve functions like AF0, AF1, ...)
/// @param config конфигурация выхода (Push_pull, Open_drain)
/// @param pull_up_down подтяжка (no_pull, pull_up, pull_down)
/// @param speed скорость работы (Low_speed, Medium_speed, High_speed, Very_high_speed, Input)
void gpio_init(GPIO_Pin gpio, GPIO_Mode mode, GPIO_Config config, GPIO_Pull pull_up_down, GPIO_Speed speed){
	GPIO_Struct gpio_struct = gpio_calculate(gpio);
    RCC->AHB2ENR |= 1 << gpio_struct.port_num;
	gpio_struct.port->PUPDR &= ~(3 << (gpio_struct.pin * 2));  // pull up / pull down register
	gpio_struct.port->PUPDR |= pull_up_down << (gpio_struct.pin * 2);

	gpio_struct.port->OTYPER &= ~(1 << gpio_struct.pin);
	gpio_struct.port->OTYPER |= (config << gpio_struct.pin);

	gpio_struct.port->OSPEEDR &= ~(3 << (gpio_struct.pin * 2));
	if(speed != Input) gpio_struct.port->OSPEEDR |= speed << (gpio_struct.pin * 2);

	if(mode > 3){
		gpio_struct.port->MODER &= ~(3 << (gpio_struct.pin * 2));
		gpio_struct.port->MODER |= 2 << (gpio_struct.pin * 2);

		gpio_struct.port->AFR[gpio_struct.pin/8] &= ~(0x0F << (gpio_struct.pin * 4));
		gpio_struct.pin/8 == 0 ? (gpio_struct.port->AFR[gpio_struct.pin/8] |= ((mode-4) << (gpio_struct.pin * 4))) : (gpio_struct.port->AFR[gpio_struct.pin/8] |= ((mode-4) << ((gpio_struct.pin-8) * 4)));
	}
	else{
		gpio_struct.port->MODER &= ~(3 << (gpio_struct.pin * 2));
		gpio_struct.port->MODER |= mode << (gpio_struct.pin * 2);
	}
}

/// @brief Устанавливает выбранный пин в состояние state (LOW or HIGH)
/// @param gpio номер вывода (PA0, PA1, ...)
/// @param state LOW or HIGH
void gpio_state(GPIO_Pin gpio, GPIO_State state){
    if(state == LOW || state == HIGH){
        GPIO_Struct gpio_struct = gpio_calculate(gpio);
        gpio_struct.port->BSRR = 0x01 << (gpio_struct.pin + (!state) * 16);
    }
}

/// @brief Инвертирует состояние пина
/// @param gpio номер вывода (PA0, PA1, ...)
void gpio_toggle(GPIO_Pin gpio){
    GPIO_Struct gpio_struct = gpio_calculate(gpio);
	gpio_struct.port->ODR ^= 0x01 << gpio_struct.pin;
}

/// @brief Возвращает логический уровень указанного пина
/// @param gpio номер вывода (PA0, PA1, ...)
/// @return LOW or HIGH
GPIO_State gpio_read(GPIO_Pin gpio){
    GPIO_Struct gpio_struct = gpio_calculate(gpio);
    return (gpio_struct.port->IDR & (0x01 << gpio_struct.pin)) >> gpio_struct.pin;
}

void gpio_exti_init(GPIO_Pin gpio, uint8_t mode){
    if(gpio != uninitialized){
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
        gpio_init(gpio, Input_mode, Open_drain, no_pull, Input);
        gpio--;
        uint8_t port_num = (gpio / 16);  // GPIOA = 0, GPIOB = 1, ..., GPIOE = 4
        uint8_t pin = gpio % 16;  // PB0 = 0, PA1 = 1, PD4 = 4
        // EXTI->IMR1 |= EXTI_IMR1_IM4;
        EXTI->IMR1 |= 1 << pin;   // generate interrupt
        // EXTI->EMR1 |= pin;   // generate event
        if(mode == 0) EXTI->RTSR1 |= 1 << pin;
        else if(mode == 1) EXTI->FTSR1 |= 1 << pin;
        else if(mode == 2){
            EXTI->RTSR1 |= 1 << pin;
            EXTI->FTSR1 |= 1 << pin;
        }
        SYSCFG->EXTICR[pin / 4] |= port_num << (4 * (pin % 4));
        if(pin < 5) NVIC_EnableIRQ(EXTI4_IRQn);
        else if(pin < 10) NVIC_EnableIRQ(EXTI9_5_IRQn);
        else if(pin < 16) NVIC_EnableIRQ(EXTI15_10_IRQn);
    }
}