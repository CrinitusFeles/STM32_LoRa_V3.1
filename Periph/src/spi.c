#include "stm32l4xx.h"

void spi_init(SPI_TypeDef *SPIx, int speed_div_, int mode_, int data_length_, int first_bit_){
	/*
	 * -----------------------------------------------
	 *  SPI1 - APB2;
	 *
	 * 	NSS -- 	PA4 AFIO PP / Input pull-up
     *  SCK -- 	PA5 AFIO PP
	 *  MISO -- PA6 Input pull-up
	 *  MOSI -- PA7 AFIO PP
	 *  ----------------------------------------------
	 *  SPI2 - APB1;
	 *
	 *  NSS -- 	PB12 AFIO PP / Input pull-up
     *  SCK -- 	PB13 AFIO PP
	 *  MISO -- PB14 Input pull-up
	 *  MOSI -- PB15 AFIO PP
	 *  ----------------------------------------------
	 *  Mode 0 - CPOL=0; CPHA=0
	 *  Mode 1 - CPOL=0; CPHA=1
	 *  Mode 2 - CPOL=1; CPHA=0
	 *  Mode 3 - CPOL=1; CPHA=1
	 */
	if(SPIx == SPI1){
		RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	}
	else if(SPIx == SPI2){
		RCC->APB1ENR1 |= RCC_APB1ENR1_SPI2EN;
	}
	else if(SPIx == SPI3){
		RCC->APB1ENR1 |= RCC_APB1ENR1_SPI3EN;
	}

	SPIx->CR1 |= speed_div_ << 3;
	SPIx->CR1 |= mode_ << 0;
	SPIx->CR1 |= first_bit_ << 7;
	SPIx->CR1 |= SPI_CR1_MSTR;  //Mode Master
	SPIx->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; //Software slave management & Internal slave select

//	SPIx->CR1 |= 0x01 << 14;
	SPIx->CR1 |= 0x00 << 11;
	SPIx->CR2 = (0x07 << SPI_CR2_DS_Pos) | SPI_CR2_FRXTH; // важно
	SPIx->CR1 |= SPI_CR1_SPE; //Enable SPI1
}
void spi_write(SPI_TypeDef *SPIx, uint8_t data)
{
	/* Write data (8 or 16 bits, depending on DFF) into DR. */
	SPIx->DR = data;
}
uint8_t spi_xfer(SPI_TypeDef *SPIx, uint8_t data)
{
	spi_write(SPIx, data);

	/* Wait for transfer finished. */
	while (!(SPIx->SR & SPI_SR_RXNE));

	/* Read the data (8 or 16 bits, depending on DFF bit) from DR. */
	return SPIx->DR;
}
uint8_t spi_txrx(SPI_TypeDef *SPIx, uint8_t data)
{
	while(!(SPIx->SR & SPI_SR_TXE));
	*(__IO uint8_t*)&(SPIx->DR) = data;
	while ((SPIx->SR & SPI_SR_RXNE) == 0);
	while (SPI1->SR & SPI_SR_BSY);

	return *(__IO uint8_t*)&(SPIx->DR);
}
void spi_send8(SPI_TypeDef *SPIx, uint8_t data){
	while(!(SPIx->SR & SPI_SR_TXE));
	*(__IO uint8_t *)(&SPIx->DR) = data;
	// while((!(*((__IO uint8_t *)&SPIx->SR) & SPI_SR_RXNE)) == SPI_SR_RXNE) {}
	(void) SPIx->DR;
	(void) SPIx->SR;
}
uint8_t spi_recieve8(SPI_TypeDef *SPIx){
	/* Set RX Fifo threshold according the reception data length: 8bit */
	while(!(SPIx->SR & SPI_SR_TXE)); // waiting while transmit buffer is not empty
	*(__IO uint8_t *)(&SPIx->DR) = 0x00;

	volatile uint32_t counter = 0;
	volatile uint8_t data = 0;
	volatile uint8_t status = 1;
	// if(SPIx->SR & SPI_SR_FRLVL == 0){
	// 	data = *((__IO uint8_t *)&SPIx->DR);
	// }
	while(1){
		if((SPIx->SR & SPI_SR_RXNE) == SPI_SR_RXNE){  // if recieve buffer is not empty then read it
			status = (SPIx->SR >> SPI_SR_FRLVL_Pos) & 0x03;
			data = *(__IO uint8_t *)(&SPIx->DR);
			if(status == 0){
				return data;
			}
		}
		else{
			counter += 1;
			if(counter > 150000){
				break;
			}
		}
		if(status == 0){
			break;
		}
	}
	// while(!(SPIx->SR & SPI_SR_RXNE));
	// data = *((__IO uint8_t *)&SPIx->DR);

	return data;
}
void spi_send_array(SPI_TypeDef *SPIx, uint8_t *data, uint8_t size){
	for(int i = 0; i < size; i++){
		spi_send8(SPIx, (uint8_t)data[i]);
	}
}
void spi_send16(SPI_TypeDef *SPIx, uint16_t data){
	uint8_t littleByte = data & 0xFF;
	uint8_t bigByte = (data>>8) & 0xFF;

	while(!(SPIx->SR & SPI_SR_TXE));
	SPIx->DR = bigByte;
	while(!(SPIx->SR & SPI_SR_TXE));
	SPIx->DR = littleByte;

}
uint8_t spi_waiting_read8(SPI_TypeDef *SPIx){
	SPIx->DR = 0;
	while(!(SPIx->SR & SPI_SR_RXNE));
	return SPIx->DR;
}
uint8_t spi_request_read8(SPI_TypeDef *SPIx){
	SPIx->DR = 0;
	while(!(SPIx->SR & SPI_SR_RXNE));
	return SPIx->DR;
}
