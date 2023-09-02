/*
 * tmp1075.c
 *
 *  Created on: 26 ����. 2020 �.
 *      Author: ftor
 */
#include "stm32l4xx.h"
#include "i2c.h"
#include "TMP1075.h"


/**	@brief	Reading device id.
	@param *I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_read_id(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data){

	return I2C_Read_word_u16_St_ReSt(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x0F, 1, read_data);
}


/**	@brief	Reading raw data from temperature register.
	@param *I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@param read_data - pointer to get data in raw format.
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_read_raw_temperature(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data){

	return I2C_Read_word_u16_St_ReSt(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x00, 1, read_data);
}

/**	@brief	Reading data from temperature register and convert to float.
	@param *I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@param temp_value - pointer to get data in float format.
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_read_float_temperature(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, float *temp_value){

	uint16_t ADC_CODE;

	if( I2C_Read_word_u16_St_ReSt(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x00, 1, &ADC_CODE) != SUCCESS){
		return -1;
	}

	*temp_value = TMP1075_raw_to_float(ADC_CODE);

	return SUCCESS;
}

/**	@brief	Reading data from temperature register and convert to int8_t (sign integer ).
	@param *I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@param temp_value - pointer to get data in int8_t format.
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_read_int8_temperature(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, int8_t *temp_value){

	uint16_t ADC_CODE;

	if( I2C_Read_word_u16_St_ReSt(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x00, 1, &ADC_CODE) != SUCCESS){
		return -1;
	}

	*temp_value = TMP1075_raw_to_int(ADC_CODE);

	return SUCCESS;
}


/**	@brief	Converter from raw data (temperature register) to float.
	@param ADC_CODE - raw data from tmp1075 sensor.
	@retval	temperature value in float format
*/
float TMP1075_raw_to_float(uint16_t ADC_CODE){

	int16_t ADC_data_int = 0;
	float ret_val = 0;

	ADC_data_int = (int16_t)ADC_CODE;

	ADC_data_int = ADC_data_int>>4;

	ret_val = (float)( ( (float)ADC_data_int ) * 0.0625 );

	return ret_val;
}


/** @brief  Converter from raw data (temperature register)  to int8_t.
	@param ADC_CODE - raw data from tmp1075 sensor.
	@retval temperature value in int8_t format
*/
int8_t TMP1075_raw_to_int(uint16_t ADC_CODE){

	float temperature = 0;

	temperature = TMP1075_raw_to_float(ADC_CODE);

	return ((int8_t)temperature);
}


/** @brief  Converter from float to raw data temperature register..
	@param 	val_temp - temperature value in float format.
	@retval raw data in tmp1075 sensor format
*/
uint16_t TMP1075_float_to_binary(float val_temp){

	int16_t ADC_data_int = 0;

	ADC_data_int = (int16_t)(val_temp/0.0625);

	ADC_data_int = (int16_t)(ADC_data_int<<4);

	return ((uint16_t)ADC_data_int);
}


/** @brief	Reading configuration register tmp1075 sensor.
	@param *I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param  tmp1075_addr - 7-bit device address.
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_read_config(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data){

	return I2C_Read_word_u16_St_ReSt(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x01, 1, read_data);
}

/** @brief	Writing configuration register tmp1075 sensor.
	@param *I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param  tmp1075_addr - 7-bit device address.
	@param  config - 8 bit of config register;
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_write_config(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t config){

	if (I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x01, config ) != SUCCESS ){
		return -1;
	}
	else{
		return SUCCESS;
	}
}

/** @brief  Sets the device in SHUTDOWN_MODE to conserve power or in CONTINUOS_CONVERSION.
	@param *I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@param mode - TMP1075_SHUTDOWN_MODE or TMP1075_CONTINUOS_CONV mode of TMP1075 sensor.
	@retval 0-OK, -1-Error
*/
ErrorStatus TMP1075_set_mode(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t mode){

	uint16_t last_state;
	uint16_t current_state;

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &last_state) != SUCCESS ){
		return -1;
	}

	if( I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x01, (uint16_t)((last_state & (~(1 << 8))) | (mode << 8)) ) != SUCCESS ){
		return -1;
	}

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &current_state) != SUCCESS ){
		return -1;
	}

	if((current_state & (~(1 << 8)))  | (mode << 8)){
		return SUCCESS;

	}else{
		return -1;
	}
}


/** @brief	Conversion rate setting when device is in continuos conversion mode
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param  tmp1075_addr - 7-bit device address.
	@param  time - conversion rate:
				 TMP1075_CR_VERY_FAST        // 27.5 ms
				 TMP1075_CR_FAST             // 55 ms
				 TMP1075_CR_MEDIUM           // 110 ms
				 TMP1075_CR_SLOW             // 220 ms
	@retval 0-OK, -1-Error
*/
ErrorStatus TMP1075_set_time_conversion(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t time){

	uint16_t last_state;
	uint16_t current_state;

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &last_state) != SUCCESS ){
		return -1;
	}

	if( I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x01, (uint16_t)((last_state & (~(3 << 13))) | (time << 13)) )!= SUCCESS ){
		return -1;
	}

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &current_state) != SUCCESS ){
		return -1;
	}

	if( ( (last_state & (~(3 << 13))) | (time << 13) ) != current_state ){
		return -1;
	}

	return SUCCESS;
}


/** @brief	One-shot conversion mode (for SHUTDOWN_MODE). Requests single measurement of temperature.
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param  tmp1075_addr - 7-bit device address.
	@retval	0-OK, 1-Error
*/
ErrorStatus TMP1075_one_shot_conversion_start(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr){

	uint16_t last_state;
	uint16_t current_state;
	volatile uint8_t errors = 0;

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &last_state) != SUCCESS ){
		errors++;
		return ERROR;
	}

	if( I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x01, (uint16_t) ((last_state & (~(1 << 15))) | (1 << 15)) ) != SUCCESS ){
		errors++;
		return ERROR;
	}

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &current_state) != SUCCESS ){
		errors++;
		return ERROR;
	}

	// if(((last_state & (~(1 << 15))) | (1 << 15)) != current_state){
	// 	errors++;
	// 	return ERROR;
	// }

	return SUCCESS;
}


/** @brief  Disable ALERT pin function.
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param  tmp1075_addr - 7-bit device address.
	@retval 0-OK, 1-Error
*/
ErrorStatus TMP1075_disable_ALERT_pin(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr){

	if( TMP1075_set_low_limit( I2Cx, tmp1075_addr, -128) != SUCCESS){
		return -1;
	}

	if( TMP1075_set_high_limit(I2Cx, tmp1075_addr, 127.9375) != SUCCESS){
		return -1;
	}

	return SUCCESS;
}


/** @brief	Selects the mode of the ALERT pin.
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param  tmp1075_addr - 7-bit device address.
	@param  mode - ALERT mode:
				 TMP1075_COMPARATOR_MODE
				 TMP1075_INTERRUPT_MODE
	@retval	0-OK, 1-Error
*/
ErrorStatus TMP1075_set_mode_ALERT_pin(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t mode){

	uint16_t last_state;
	uint16_t current_state;

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &last_state) != SUCCESS ){
		return -1;
	}

	if( I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x01,  (uint16_t)(( last_state & (~(1 << 9)) ) | (mode << 9))  ) != SUCCESS ){
		return -1;
	}

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &current_state) != SUCCESS ){
		return -1;
	}

	if( ( (last_state & (~(1 << 9))) | (mode << 9) ) != current_state ){

		return -1;
	}

	return SUCCESS;
}


/** @brief	Polarity of the output ALERT pin.
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@param mode - ALERT polarity.
				 TMP1075_ACTIVE_LOW
				 TMP1075_ACTIVE_HIGH
	@retval	0-OK, 1-Error
*/
ErrorStatus TMP1075_ALERT_active_level(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t mode){

	uint16_t last_state;
	uint16_t current_state;

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &last_state) != SUCCESS ){
		return -1;
	}

	if( I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x01, (uint16_t)(( last_state & (~(1 << 10))) | (mode << 10)) ) != SUCCESS ){
		return -1;
	}

	if( TMP1075_read_config( I2Cx, tmp1075_addr, &current_state ) != SUCCESS ){
		return -1;
	}

	if( ( (last_state & (~(1 << 10))) | (mode << 10) ) != current_state ){
		return -1;
	}

	return SUCCESS;
}


/** @brief	Consecutive fault measurements to trigger the alert function.
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@param mode - amount of faults:
				 TMP1075_ONE_FAULT
				 TMP1075_TWO_FAULT
				 TMP1075_THREE_FAULT
				 TMP1075_FOUR_FAULT
	@retval	0-OK, 1-Error
*/
ErrorStatus TMP1075_ALERT_sensitivity(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint8_t mode){

	uint16_t last_state;
	uint16_t current_state;

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &last_state) != SUCCESS ){
		return -1;
	}

	if( I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x01, (uint16_t)((last_state & (~(3 << 11))) | (mode<< 11)) ) != SUCCESS ) {
		return -1;
	}

	if( TMP1075_read_config(I2Cx, tmp1075_addr, &current_state) != SUCCESS ){
		return -1;
	}

	if(((last_state & (~(3 << 11))) | (mode << 11)) != current_state){
		return -1;
	}

	return SUCCESS;
}


/** @brief	Set low limit for comparison with temperature results.
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@param low_limit -  low limit for comparison with temperature results.
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_set_low_limit(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, float low_limit){

	uint16_t low_limit_temp=0;
	uint16_t get_reg_data=0;

	low_limit_temp = TMP1075_float_to_binary(low_limit);

	if( I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x02, low_limit_temp) != SUCCESS ){
		return -1;
	}

	if( TMP1075_get_low_limit(I2Cx, tmp1075_addr, &get_reg_data) != SUCCESS ){
		return -1;
	}

	if( low_limit_temp != get_reg_data ){
		return -1;
	}

	return SUCCESS;
}


/** @brief  Set high limit for comparison with temperature results
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@param high_limit - high limit for comparison with temperature results.
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_set_high_limit(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, float high_limit){

	uint16_t high_limit_temp=0;
	uint16_t get_reg_data=0;

	high_limit_temp = TMP1075_float_to_binary(high_limit);

	if( I2C_Write_word_u16_St(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x03, high_limit_temp) != SUCCESS ){
		return -1;
	}

	if( TMP1075_get_high_limit(I2Cx, tmp1075_addr, &get_reg_data) != SUCCESS ){
		return -1;
	}

	if( high_limit_temp != get_reg_data ){
		return -1;
	}

	return SUCCESS;
}


/** @brief  Reading current value of low limit register.
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@retval 0-OK, -1-Error
*/
ErrorStatus TMP1075_get_low_limit(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data){

	return I2C_Read_word_u16_St_ReSt(I2Cx, tmp1075_addr,  I2C_SIZE_REG_ADDR_U8, (uint32_t)0x02, 1, read_data);
}


/** @brief	Reading current value of high limit register.
	@param  I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param tmp1075_addr - 7-bit device address.
	@retval	0-OK, -1-Error
*/
ErrorStatus TMP1075_get_high_limit(I2C_TypeDef *I2Cx, uint8_t tmp1075_addr, uint16_t *read_data){

	return I2C_Read_word_u16_St_ReSt(I2Cx, tmp1075_addr, I2C_SIZE_REG_ADDR_U8, (uint32_t)0x03, 1, read_data);
}

