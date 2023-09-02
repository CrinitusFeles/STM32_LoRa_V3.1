#include "i2c.h"
#include "stm32l4xx.h"


void I2C_init(I2C_TypeDef *I2Cx){
	if(I2Cx == I2C1){
		RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;
	}
	else if(I2Cx == I2C2){
		RCC->APB1ENR1 |= RCC_APB1ENR1_I2C2EN;
	}
	else if(I2Cx == I2C3){
		RCC->APB1ENR1 |= RCC_APB1ENR1_I2C3EN;
	}
	else{
		return;
	}

	I2Cx->CR2 |= I2C_CR2_AUTOEND;
	I2Cx->OAR2 &= ~I2C_OAR2_OA2EN;
	I2Cx->CR1 &= ~I2C_CR1_GCEN;
	I2Cx->CR1 &= ~I2C_CR1_NOSTRETCH;
	// I2Cx->TIMINGR = 0x00702991; //Sysclk = 80 MHz I2Cmode = fast (400kHz)
	// I2Cx->TIMINGR = 0x00000004; //Sysclk = 4 MHz  I2Cmode = fast (400kHz)
	I2Cx->TIMINGR = 0x00000E14; //Sysclk = 4 MHz  I2Cmode = standart (100kHz)


	(((I2Cx->OAR2)) = ((((((I2Cx->OAR2))) & (~((0x7FUL << (1U)) | (0x7UL << (8U))))) | (0 | 0))));
	I2Cx->CR1 |= I2C_CR1_PE;
}

/**
  * @brief  Handles I2Cx communication when starting transfer or during transfer (TC or TCR flag are set).
  * @rmtoll CR2          SADD          LL_I2C_HandleTransfer\n
  *         CR2          ADD10         LL_I2C_HandleTransfer\n
  *         CR2          RD_WRN        LL_I2C_HandleTransfer\n
  *         CR2          START         LL_I2C_HandleTransfer\n
  *         CR2          STOP          LL_I2C_HandleTransfer\n
  *         CR2          RELOAD        LL_I2C_HandleTransfer\n
  *         CR2          NBYTES        LL_I2C_HandleTransfer\n
  *         CR2          AUTOEND       LL_I2C_HandleTransfer\n
  *         CR2          HEAD10R       LL_I2C_HandleTransfer
  * @param  I2Cx I2C Instance.
  * @param  SlaveAddr Specifies the slave address to be programmed.
  * @param  SlaveAddrSize This parameter can be one of the following values:
  *         @arg @ref LL_I2C_ADDRSLAVE_7BIT
  *         @arg @ref LL_I2C_ADDRSLAVE_10BIT
  * @param  TransferSize Specifies the number of bytes to be programmed.
  *                       This parameter must be a value between Min_Data=0 and Max_Data=255.
  * @param  EndMode This parameter can be one of the following values:
  *         @arg @ref LL_I2C_MODE_RELOAD
  *         @arg @ref LL_I2C_MODE_AUTOEND
  *         @arg @ref LL_I2C_MODE_SOFTEND
  *         @arg @ref LL_I2C_MODE_SMBUS_RELOAD
  *         @arg @ref LL_I2C_MODE_SMBUS_AUTOEND_NO_PEC
  *         @arg @ref LL_I2C_MODE_SMBUS_SOFTEND_NO_PEC
  *         @arg @ref LL_I2C_MODE_SMBUS_AUTOEND_WITH_PEC
  *         @arg @ref LL_I2C_MODE_SMBUS_SOFTEND_WITH_PEC
  * @param  Request This parameter can be one of the following values:
  *         @arg @ref LL_I2C_GENERATE_NOSTARTSTOP
  *         @arg @ref LL_I2C_GENERATE_STOP
  *         @arg @ref LL_I2C_GENERATE_START_READ
  *         @arg @ref LL_I2C_GENERATE_START_WRITE
  *         @arg @ref LL_I2C_GENERATE_RESTART_7BIT_READ
  *         @arg @ref LL_I2C_GENERATE_RESTART_7BIT_WRITE
  *         @arg @ref LL_I2C_GENERATE_RESTART_10BIT_READ
  *         @arg @ref LL_I2C_GENERATE_RESTART_10BIT_WRITE
  * @retval None
  */
void I2C_HandleTransfer(I2C_TypeDef *I2Cx, uint32_t SlaveAddr, uint32_t SlaveAddrSize,
                                           uint32_t TransferSize, uint32_t EndMode, uint32_t Request)
{
  MODIFY_REG(I2Cx->CR2, I2C_CR2_SADD | I2C_CR2_ADD10 | (I2C_CR2_RD_WRN & (uint32_t)(Request >> (31U - I2C_CR2_RD_WRN_Pos))) | I2C_CR2_START | I2C_CR2_STOP | I2C_CR2_RELOAD |
             I2C_CR2_NBYTES | I2C_CR2_AUTOEND | I2C_CR2_HEAD10R,
             SlaveAddr | SlaveAddrSize | (TransferSize << I2C_CR2_NBYTES_Pos) | EndMode | Request);
}

ErrorStatus I2C_check_flag(uint8_t checked_flag, uint8_t flag_state){
	volatile int32_t counter = I2C_TIMEOUT;
	while(checked_flag == flag_state && (counter--) >= 0);
	if(counter == 0){
		return ERROR;
	}
	return SUCCESS;
}

void I2C_Clear_Error_Flags(I2C_TypeDef *I2Cx){

	I2Cx->ISR = I2C_ISR_TXE;
	I2Cx->ICR |= I2C_ICR_NACKCF;
	I2Cx->ICR |= I2C_ICR_BERRCF;
	I2Cx->ICR |= I2C_ICR_STOPCF;

	if((I2Cx->ISR & I2C_ISR_ARLO) == 1){
		I2Cx->ISR |= I2C_ISR_ARLO;
	}
	if((I2Cx->ISR & I2C_ISR_BUSY) == 1){
		I2Cx->CR1 &= ~I2C_CR1_PE;
		I2Cx->CR1 |= I2C_CR1_PE;
	}
}

/** @brief	Reading single byte  ( St_ReSt - generate Start and Restart )
	@param 	*I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param 	SlaveAddr - 8-bit device address.
	@param  size_reg_addr - size of reg_addr in byte if:
				reg_addr = U8 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U8
				reg_addr = U16 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U16
				reg_addr = U24 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U24
				reg_addr = U32 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U32
	@param 	reg_addr - 8,16,24,32-bit Registry address on the remote device
	@param  *data - pointer to variable u8 where would be written data from the remote device.
	@retval 0 - SUCCESS, -1 - ERROR
*/
ErrorStatus I2C_Read_byte_St_ReSt(I2C_TypeDef *I2Cx, uint8_t SlaveAddr, uint8_t size_reg_addr , uint32_t reg_addr, uint8_t *data){

	if( size_reg_addr == 0 || size_reg_addr > 4 ){
		return ERROR;
	}
	volatile int32_t timeout = I2C_TIMEOUT;
	uint8_t receive_data = 0;
	uint8_t i = 0;
	int8_t  j = 0;
	uint32_t SlaveAddr1;

	SlaveAddr1 = (uint8_t)(SlaveAddr << 1);

	//Clear flags if the previous attempt to exchange was not successful.
	I2C_Clear_Error_Flags(I2Cx);

	I2C_HandleTransfer(I2Cx, (uint32_t)SlaveAddr1, I2C_ADDRSLAVE_7BIT, (uint32_t)size_reg_addr, I2C_MODE_SOFTEND, I2C_GENERATE_START_WRITE);
	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}

	for( i = size_reg_addr , j = size_reg_addr-1 ; i != 0; i--, j-- ){ //high byte is sent first

		I2Cx->TXDR = (uint8_t)(reg_addr >> (j*8));

		while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
		if(timeout <= 0){
			return ERROR;
		}
	}

	while(((I2Cx->ISR & I2C_ISR_TC) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}

	I2C_HandleTransfer(I2Cx, (uint32_t)SlaveAddr1, I2C_ADDRSLAVE_7BIT, (uint32_t)1, I2C_MODE_AUTOEND, I2C_GENERATE_START_READ);

	while(((I2Cx->ISR & I2C_ISR_RXNE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	//LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_NACK);
	receive_data = I2Cx->RXDR & 0xFF;

	//LL_I2C_GenerateStopCondition(I2Cx);
	while(((I2Cx->ISR & I2C_ISR_STOPF) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}

	I2Cx->ICR |= I2C_ICR_STOPCF;

	*data = receive_data;

	return SUCCESS;
}


/** @brief	Reading two byte register. ( St_ReSt - generate Start and Restart )
	@param 	*I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param 	SlaveAddr - 8-bit device address.
	@param  size_reg_addr - size of reg_addr in byte if:
				reg_addr = U8 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U8
				reg_addr = U16 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U16
				reg_addr = U24 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U24
				reg_addr = U32 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U32
	@param 	reg_addr - 8,16,24,32-bit Registry address on the remote device
	@param 	*data - pointer to variable u16 where would be written data from the remote device.
	@retval 0 - SUCCESS, -1 - ERROR
*/
ErrorStatus I2C_Read_word_u16_St_ReSt(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t size_reg_addr, uint32_t reg_addr, uint8_t msb_first, uint16_t *data){
	volatile int32_t timeout = I2C_TIMEOUT;
	if( size_reg_addr == 0 || size_reg_addr > 4 ){
		return ERROR;
	}

	uint8_t high_byte = 0, low_byte = 0;
	uint8_t i = 0;
	int8_t  j = 0;

	dev_addr = (uint8_t)(dev_addr << 1);

	//Clear flags if the previous attempt to exchange was not successful.
	I2C_Clear_Error_Flags(I2Cx);
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_BUSY) >> I2C_ISR_BUSY_Pos, SET) != SUCCESS){
	// 	return ERROR;
	// }
	while(((I2Cx->ISR & I2C_ISR_BUSY) == SET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}

	I2C_HandleTransfer(I2Cx, (uint32_t)dev_addr, I2C_ADDRSLAVE_7BIT, (uint32_t)size_reg_addr, I2C_MODE_SOFTEND, I2C_GENERATE_START_WRITE);
	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }
	I2Cx->CR2 |= I2C_CR2_START;

	for( i = size_reg_addr , j = size_reg_addr-1 ; i != 0; i--, j-- ){ //high byte is sent first

		I2Cx->TXDR = (uint8_t)(reg_addr >> (j*8));

		while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
		if(timeout <= 0){
			return ERROR;
		}
		// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
		// 	return ERROR;
		// }
	}

	while(((I2Cx->ISR & I2C_ISR_TC) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TC) >> I2C_ISR_RXNE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	I2C_HandleTransfer(I2Cx, (uint32_t)dev_addr, I2C_ADDRSLAVE_7BIT, (uint32_t)2, I2C_MODE_AUTOEND, I2C_GENERATE_RESTART_7BIT_READ); //LL_I2C_MODE_SOFTEND

	while(((I2Cx->ISR & I2C_ISR_RXNE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_RXNE) >> I2C_ISR_RXNE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	//LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);
	high_byte = I2Cx->RXDR & 0xFF;

	while(((I2Cx->ISR & I2C_ISR_RXNE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_RXNE) >> I2C_ISR_RXNE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }
	// LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);
	low_byte = I2Cx->RXDR & 0xFF;

	while(((I2Cx->ISR & I2C_ISR_STOPF) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	//LL_I2C_GenerateStopCondition(I2Cx);
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_STOPF) >> I2C_ISR_STOPF_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	I2Cx->ICR |= I2C_ICR_STOPCF;
	if(msb_first){
		*data = (uint16_t)( ( high_byte  << 8 ) | low_byte );
	}
	else{
		*data = (uint16_t)( ( low_byte  << 8 ) |  high_byte);
	}


	return SUCCESS;
}


ErrorStatus I2C_Read_word_u24_St_ReSt(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t size_reg_addr, uint32_t reg_addr, uint32_t *data){
	volatile int32_t timeout = I2C_TIMEOUT;
	if( size_reg_addr == 0 || size_reg_addr > 4 ){
		return ERROR;
	}

	uint8_t high_byte = 0, mid_byte = 0, low_byte = 0;
	uint8_t i = 0;
	int8_t  j = 0;

	dev_addr = (uint8_t)(dev_addr << 1);

	//Clear flags if the previous attempt to exchange was not successful.
	I2C_Clear_Error_Flags(I2Cx);
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_BUSY) >> I2C_ISR_BUSY_Pos, SET) != SUCCESS){
	// 	return ERROR;
	// }
	while(((I2Cx->ISR & I2C_ISR_BUSY) == SET) && ((timeout--) >= 0));
	if(timeout <= 0){
		return ERROR;
	}

	I2C_HandleTransfer(I2Cx, (uint32_t)dev_addr, I2C_ADDRSLAVE_7BIT, (uint32_t)size_reg_addr, I2C_MODE_SOFTEND, I2C_GENERATE_START_WRITE);
	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && ((timeout--) >= 0));
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }
	I2Cx->CR2 |= I2C_CR2_START;

	for( i = size_reg_addr , j = size_reg_addr-1 ; i != 0; i--, j-- ){ //high byte is sent first

		I2Cx->TXDR = (uint8_t)(reg_addr >> (j*8));

		while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && ((timeout--) >= 0));
		if(timeout <= 0){
			return ERROR;
		}
		// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
		// 	return ERROR;
		// }
	}

	while(((I2Cx->ISR & I2C_ISR_TC) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TC) >> I2C_ISR_RXNE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	I2C_HandleTransfer(I2Cx, (uint32_t)dev_addr, I2C_ADDRSLAVE_7BIT, (uint32_t)3, I2C_MODE_AUTOEND, I2C_GENERATE_RESTART_7BIT_READ); //LL_I2C_MODE_SOFTEND

	while(((I2Cx->ISR & I2C_ISR_RXNE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_RXNE) >> I2C_ISR_RXNE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	//LL_I2C_AcknowledgeNextData(I2Cx, LL_I2C_ACK);
	high_byte = I2Cx->RXDR & 0xFF;

	while(((I2Cx->ISR & I2C_ISR_RXNE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	mid_byte = I2Cx->RXDR & 0xFF;

	while(((I2Cx->ISR & I2C_ISR_RXNE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	low_byte = I2Cx->RXDR & 0xFF;

	while(((I2Cx->ISR & I2C_ISR_STOPF) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	//LL_I2C_GenerateStopCondition(I2Cx);
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_STOPF) >> I2C_ISR_STOPF_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	I2Cx->ICR |= I2C_ICR_STOPCF;

	*data = (uint32_t)( ( high_byte  << 16 ) | ( mid_byte  << 8 ) | low_byte );

	return SUCCESS;
}

/**@brief	Writing uint16_t data  by address reg_addr. (St genereate only one start)
	@param 	*I2Cx - pointer to I2C controller, where x is a number (e.x., I2C1, I2C2 etc.).
	@param 	SlaveAddr - 8-bit device address.
	@param  size_reg_addr - size of reg_addr in byte if:
				reg_addr = U8 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U8
				reg_addr = U16 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U16
				reg_addr = U24 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U24
				reg_addr = U32 	-> 	size_reg_addr = I2C_SIZE_REG_ADDR_U32
	@param 	reg_addr - 8,16,24,32-bit Registry address on the remote device
	@param  data - uint16_t data to be writing
	@retval 0 - SUCCESS, -1 - ERROR
*/
ErrorStatus I2C_Write_word_u16_St(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t size_reg_addr, uint32_t reg_addr, uint16_t data){
	volatile int32_t timeout = I2C_TIMEOUT;
	if( size_reg_addr == 0 || size_reg_addr > 4 ){
		return ERROR;
	}

	uint8_t low_byte = (uint8_t) data;
	uint8_t high_byte = (uint8_t)(data >> 8);
	uint8_t i = 0;
	int8_t  j = 0;

	dev_addr = (uint8_t)(dev_addr << 1);

	//Clear flags if the previous attempt to exchange was not successful.
	I2C_Clear_Error_Flags(I2Cx);
	while(((I2Cx->ISR & I2C_ISR_BUSY) == SET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_BUSY), SET) != SUCCESS){
	// 	return ERROR;
	// }

	I2C_HandleTransfer(I2Cx, (uint32_t)dev_addr, I2C_ADDRSLAVE_7BIT, (uint32_t)(size_reg_addr+2), I2C_MODE_AUTOEND , I2C_GENERATE_START_WRITE); ////LL_I2C_MODE_SOFTEND

	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	for( i = size_reg_addr , j = size_reg_addr-1 ; i != 0; i--, j-- ){ //high byte is sent first

		I2Cx->TXDR = (uint8_t)(reg_addr >> (j*8));

		while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
		if(timeout <= 0){
			return ERROR;
		}
		// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
		// 	return ERROR;
		// }
	}

	I2Cx->TXDR = high_byte;
	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	I2Cx->TXDR = low_byte;
	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	// while(((I2Cx->ISR & I2C_ISR_TC) == RESET) && (timeout--) >= 0);
	// if(timeout <= 0){
	// 	return ERROR;
	// }

	//LL_I2C_GenerateStopCondition(I2Cx);
	while(((I2Cx->ISR & I2C_ISR_STOPF) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_STOPF) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	I2Cx->ICR |= I2C_ICR_STOPCF;

	return SUCCESS;
}

ErrorStatus I2C_Write_byte_St(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t size_reg_addr, uint32_t reg_addr, uint8_t data){
	volatile int32_t timeout = I2C_TIMEOUT;
	if( size_reg_addr == 0 || size_reg_addr > 4 ){
		return ERROR;
	}

	uint8_t i = 0;
	int8_t  j = 0;

	dev_addr = (uint8_t)(dev_addr << 1);

	//Clear flags if the previous attempt to exchange was not successful.
	I2C_Clear_Error_Flags(I2Cx);
	while(((I2Cx->ISR & I2C_ISR_BUSY) == SET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_BUSY), SET) != SUCCESS){
	// 	return ERROR;
	// }

	I2C_HandleTransfer(I2Cx, (uint32_t)dev_addr, I2C_ADDRSLAVE_7BIT, (uint32_t)(size_reg_addr+2), I2C_MODE_AUTOEND , I2C_GENERATE_START_WRITE); ////LL_I2C_MODE_SOFTEND

	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	for( i = size_reg_addr , j = size_reg_addr-1 ; i != 0; i--, j-- ){ //high byte is sent first

		I2Cx->TXDR = (uint8_t)(reg_addr >> (j*8));

		while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
		if(timeout <= 0){
			return ERROR;
		}
		// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_TXE) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
		// 	return ERROR;
		// }
	}

	I2Cx->TXDR = data;
	while(((I2Cx->ISR & I2C_ISR_TXE) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	//LL_I2C_GenerateStopCondition(I2Cx);
	I2Cx->CR2 |= I2C_CR2_STOP;
	while(((I2Cx->ISR & I2C_ISR_STOPF) == RESET) && (timeout--) >= 0);
	if(timeout <= 0){
		return ERROR;
	}
	// if(I2C_check_flag((I2Cx->ISR & I2C_ISR_STOPF) >> I2C_ISR_TXE_Pos, RESET) != SUCCESS){
	// 	return ERROR;
	// }

	I2Cx->ICR |= I2C_ICR_STOPCF;

	return SUCCESS;
}
