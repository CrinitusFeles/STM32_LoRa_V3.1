/*
 * spi.h
 *
 *  Created on: 1 ����. 2020 �.
 *      Author: BreakingBad
 */

#ifndef INC_SPI_H_
#define INC_SPI_H_
#include "stm32l4xx.h"
#define SPI_CS		PA4
#define SPI_SCK		PA5
#define SPI_MISO	PA6
#define SPI_MOSI	PA7

enum spi_mode{
	Mode_0, Mode_1, Mode_2, Mode_3
};
enum spi_data_legth{
	data_8_bit, data_16_bit
};
enum spi_first_bit_mode{
	MSB, LSB
};
enum spi_speed_divider {
	div_2, div_4, div_8, div_16, div_32, div_64, div_128, div_256
};

void spi_send8(SPI_TypeDef *SPIx, uint8_t data);
uint8_t spi_recieve8(SPI_TypeDef *SPIx);
uint8_t spi_txrx(SPI_TypeDef *SPIx, uint8_t data);
uint8_t spi_xfer(SPI_TypeDef *SPIx, uint8_t data);
void spi_write(SPI_TypeDef *SPIx, uint8_t data);
void spi_send_array(SPI_TypeDef *SPIx, uint8_t *data, uint8_t size);
void spi_send16(SPI_TypeDef *SPIx, uint16_t data);
uint8_t spi_waiting_read8(SPI_TypeDef *SPIx);
uint8_t spi_request_read8(SPI_TypeDef *SPIx);
void spi_init(SPI_TypeDef *SPIx, int speed_div_, int mode_, int data_length_, int first_bit_);

#endif /* INC_SPI_H_ */
