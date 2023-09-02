/*
 * tmp1075.h
 *
 *  Created on: 26 сент. 2020 г.
 *      Author: ftor
 */

#ifndef INC_TMP1075_H_
#define INC_TMP1075_H_

#include "stm32l4xx.h"


#define TMP1075_SHUTDOWN_MODE            1
#define TMP1075_CONTINUOS_CONV		     0

#define TMP1075_CR_VERY_FAST	0  // 27.5 ms
#define TMP1075_CR_FAST    		1  // 55 ms
#define TMP1075_CR_MEDIUM    	2  // 110 ms
#define TMP1075_CR_SLOW    		3  // 220 ms

#define TMP1075_ACTIVE_LOW 		0
#define TMP1075_ACTIVE_HIGH 	1

#define TMP1075_ONE_FAULT   	0
#define TMP1075_TWO_FAULT   	1
#define TMP1075_THREE_FAULT 	3
#define TMP1075_FOUR_FAULT  	4


ErrorStatus TMP1075_read_id(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data);
ErrorStatus TMP1075_read_raw_temperature(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data);
ErrorStatus TMP1075_read_float_temperature(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, float *temp_value);
ErrorStatus TMP1075_read_int8_temperature(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, int8_t *temp_value);
float  TMP1075_raw_to_float(uint16_t ADC_CODE);
int8_t TMP1075_raw_to_int(uint16_t ADC_CODE);
uint16_t TMP1075_float_to_binary(float val);
ErrorStatus TMP1075_read_config(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data);
ErrorStatus TMP1075_write_config(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t config);
ErrorStatus TMP1075_set_mode(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t mode);
ErrorStatus TMP1075_set_time_conversion(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t time);
ErrorStatus TMP1075_one_shot_conversion_start(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr);
ErrorStatus TMP1075_disable_ALERT_pin(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr);
ErrorStatus TMP1075_set_mode_ALERT_pin(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t mode);
ErrorStatus TMP1075_ALERT_active_level(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t mode);
ErrorStatus TMP1075_ALERT_sensitivity(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t mode);
ErrorStatus TMP1075_set_low_limit(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, float low_limit);
ErrorStatus TMP1075_set_high_limit(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, float high_limit);
ErrorStatus TMP1075_get_low_limit(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data);
ErrorStatus TMP1075_get_high_limit(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data);


#endif /* INC_TMP1075_H_ */
