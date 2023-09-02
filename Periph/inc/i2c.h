/*
 * i2c.h
 *
 *  Created on: 25 ����. 2020 �.
 *      Author: Gandalf
 */

#ifndef INC_I2C_H_
#define INC_I2C_H_

#include "stm32l4xx.h"
#include "System.h"

#define I2C_ADDRSLAVE_7BIT				0
#define I2C_MODE_SOFTEND				0
#define I2C_GENERATE_START_WRITE		(uint32_t)(0x80000000U | (0x1UL << (13U)))
#define I2C_MODE_AUTOEND				(0x1UL << (25U))
#define I2C_GENERATE_RESTART_7BIT_READ	(uint32_t)(0x80000000U | (0x1UL << (13U)) | (0x1UL << (10U)))
#define I2C_GENERATE_START_READ      (uint32_t)(0x80000000U | I2C_CR2_START | I2C_CR2_RD_WRN)

#define I2C_SIZE_REG_ADDR_U8  	1
#define I2C_SIZE_REG_ADDR_U16  	2
#define I2C_SIZE_REG_ADDR_U24  	3
#define I2C_SIZE_REG_ADDR_U32  	4


#define I2C_TIMEOUT 5000 // 400kHz - 150, 100kHz - 600.

void I2C_init(I2C_TypeDef *I2Cx);
void I2C_HandleTransfer(I2C_TypeDef *I2Cx, uint32_t SlaveAddr, uint32_t SlaveAddrSize,
                                           uint32_t TransferSize, uint32_t EndMode, uint32_t Request);
ErrorStatus I2C_Read_byte_St_ReSt(I2C_TypeDef *I2Cx, uint8_t SlaveAddr, uint8_t size_reg_addr , uint32_t reg_addr, uint8_t *data);
ErrorStatus I2C_Read_word_u16_St_ReSt(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t size_reg_addr, uint32_t reg_addr, uint8_t msb_first, uint16_t *data);
ErrorStatus I2C_Read_word_u24_St_ReSt(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t size_reg_addr, uint32_t reg_addr, uint32_t *data);
void I2C_Clear_Error_Flags(I2C_TypeDef *I2Cx);
ErrorStatus I2C_check_flag(uint8_t checked_flag, uint8_t flag_state);
ErrorStatus I2C_Write_word_u16_St(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t size_reg_addr, uint32_t reg_addr, uint16_t data);
ErrorStatus I2C_Write_byte_St(I2C_TypeDef *I2Cx, uint8_t dev_addr, uint8_t size_reg_addr, uint32_t reg_addr, uint8_t data);
#endif /* INC_I2C_H_ */
