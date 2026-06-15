#include "sx127x_misc.h"
#include "gpio.h"
#include "spi.h"
// #include "delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"


void LoRa_Delay(SX127x* _LoRa, uint32_t milli){
    _LoRa->base.delay(milli);
}

void LoRa_reset(SX127x* _LoRa){
	gpio_state(EN_LORA, HIGH);
	LoRa_Delay(_LoRa, 1);
	gpio_state(EN_LORA, LOW);
	LoRa_Delay(_LoRa, 6);
}



uint8_t SX127x_Read(SX127x* _LoRa, uint8_t address){
	address &= ~(1 << 7);
	gpio_state(LoRa_NSS, LOW);
	spi_txrx(_LoRa->base.spi, address);
	volatile uint8_t output = spi_txrx(_LoRa->base.spi, 0x00);
	gpio_state(LoRa_NSS, HIGH);
	return output;
}

void SX127x_Write(SX127x* _LoRa, uint8_t address, uint8_t* values, uint8_t w_length){
    address |= (1 << 7);
	gpio_state(LoRa_NSS, LOW);
	spi_txrx(_LoRa->base.spi, address);
	for(uint8_t i = 0; i < w_length; i++){
	    spi_txrx(_LoRa->base.spi, values[i]);
    }
	gpio_state(LoRa_NSS, HIGH);
}