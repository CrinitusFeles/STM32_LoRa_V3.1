#include "sx127x_misc.h"
#include "gpio.h"
#include "spi.h"
// #include "delay.h"
#include "FreeRTOS.h"
#include "task.h"

void LoRa_Delay(LoRa* _LoRa, uint32_t milli){
    _LoRa->delay(milli);
}

void LoRa_reset(LoRa* _LoRa){
	gpio_state(_LoRa->reset_pin, HIGH);
	LoRa_Delay(_LoRa, 1);
	gpio_state(_LoRa->reset_pin, LOW);
	LoRa_Delay(_LoRa, 6);
}



uint8_t LoRa_readRegister(LoRa* _LoRa, uint8_t address){
	address &= ~(1 << 7);
	gpio_state(_LoRa->CS_pin, LOW);
	LoRa_Delay(_LoRa, 1);
	spi_txrx(_LoRa->LoRaSPI, address);
	volatile uint8_t output = spi_txrx(_LoRa->LoRaSPI, 0x00);
	LoRa_Delay(_LoRa, 1);
	gpio_state(_LoRa->CS_pin, HIGH);
	LoRa_Delay(_LoRa, 2);
	return output;
}

void LoRa_writeRegister(LoRa* _LoRa, uint8_t address, volatile uint8_t data){
	address |= (1 << 7);
	gpio_state(_LoRa->CS_pin, LOW);
	LoRa_Delay(_LoRa, 1);
	spi_txrx(_LoRa->LoRaSPI, address);
	(void)spi_txrx(_LoRa->LoRaSPI, data);
	LoRa_Delay(_LoRa, 1);
	gpio_state(_LoRa->CS_pin, HIGH);
	LoRa_Delay(_LoRa, 2);
}

void LoRa_writeRegisters(LoRa* _LoRa, uint8_t address, uint8_t* values, uint8_t w_length){
    address |= (1 << 7);
	gpio_state(_LoRa->CS_pin, LOW);
	LoRa_Delay(_LoRa, 1);
	spi_txrx(_LoRa->LoRaSPI, address);
	for(uint8_t i = 0; i < w_length; i++){
	    spi_txrx(_LoRa->LoRaSPI, values[i]);
    }
	LoRa_Delay(_LoRa, 1);
	gpio_state(_LoRa->CS_pin, HIGH);
	LoRa_Delay(_LoRa, 2);
}