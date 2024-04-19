#ifndef INC_SX127x_MISC_H
#define INC_SX127x_MISC_H

#include "stm32l4xx.h"
#include "sx127x.h"

void LoRa_Delay(LoRa* _LoRa, uint32_t milli);
void LoRa_reset(LoRa* _LoRa);
uint8_t LoRa_readRegister(LoRa* _LoRa, uint8_t address);
void LoRa_writeRegister(LoRa* _LoRa, uint8_t address, uint8_t data);
void LoRa_writeRegisters(LoRa* _LoRa, uint8_t address, uint8_t* values, uint8_t w_length);

#endif