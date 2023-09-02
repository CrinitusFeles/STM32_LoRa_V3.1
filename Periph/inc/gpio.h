#ifndef INCLUDE_GPIO_H_
#define INCLUDE_GPIO_H_


#ifdef STM32F405xx
	#include "stm32f4xx.h"
	#include "f405_gpio_alt_func.h"
#endif
#ifdef STM32L496xx
	#include "stm32l4xx.h"
	#include "l496_gpio_alt_func.h"
#endif
#ifdef STM32L431xx
	#include "stm32l4xx.h"
	#include "l431_gpio_alt_func.h"
#endif

typedef enum GPIO_Errors {
	PORT_ERROR=-3, MODE_ERROR, SPEED_ERROR
} GPIO_Errors;

typedef enum GPIO_Pin {
    uninitialized,
	PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
	PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
	PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
	PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
	PE0, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,
	PF0, PF1, PF2, PF3, PF4, PF5, PF6, PF7, PF8, PF9, PF10, PF11, PF12, PF13, PF14, PF15,
	PG0, PG1, PG2, PG3, PG4, PG5, PG6, PG7, PG8, PG9, PG10, PG11, PG12, PG13, PG14, PG15,
    PH0, PH1
} GPIO_Pin;

/// @brief Состояние пина (LOW, HIGH)
typedef enum GPIO_State {
	LOW, HIGH
} GPIO_State;

/**
 * @brief GPIO_Mode variable
 * Possible values: Input_mode, General_output, Analog_mode, or Alternatuve functions like AF0, AF1, ...
 */
typedef enum GPIO_Mode {
	Input_mode, General_output, FORBIDDEN_VALUE, Analog_mode, AF0, AF1, AF2, AF3, AF4, AF5, AF6,
    AF7, AF8, AF9, AF10, AF11, AF12, AF13, AF14, AF15
} GPIO_Mode;

/**
 * @brief GPIO_Config variable
 * Possible values: Push_pull, Open_drain
 */
typedef enum GPIO_Config{
	Push_pull, Open_drain
} GPIO_Config;

/**
 * @brief GPIO_Pull variable
 * Possible values: no_pull, pull_up, pull_down
 */
typedef enum GPIO_Pull{
	no_pull, pull_up, pull_down
} GPIO_Pull;

/**
 * @brief GPIO_Speed variable
 * Possible values: Low_speed, Medium_speed, High_speed, Very_high_speed, Input
 */
typedef enum GPIO_Speed {
	Low_speed, Medium_speed, High_speed, Very_high_speed, Input
} GPIO_Speed;

void gpio_init(GPIO_Pin gpio, GPIO_Mode mode, GPIO_Config config, GPIO_Pull pull_up_down, GPIO_Speed speed);
void gpio_state(GPIO_Pin gpio, GPIO_State state);
void gpio_toggle(GPIO_Pin gpio);
GPIO_State gpio_read(GPIO_Pin gpio);

/**
 * @brief Init EXTI interrupt for GPIO
 *
 * @param gpio GPIO pin: PA0, PB4, PC15 ...
 * @param mode 0 - rising edge, 1 - falling edge, 2 - rising and fallings edges
 */
void gpio_exti_init(GPIO_Pin gpio, uint8_t mode);

#endif /* INCLUDE_GPIO_H_ */


