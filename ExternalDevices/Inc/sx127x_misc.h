#ifndef INC_SX127x_MISC_H
#define INC_SX127x_MISC_H

#include "stm32l4xx.h"
#include "sx127x.h"

void LoRa_Delay(SX127x* _LoRa, uint32_t milli);
void LoRa_reset(SX127x* _LoRa);
uint8_t SX127x_Read(SX127x* _LoRa, uint8_t address);
void SX127x_Write(SX127x* _LoRa, uint8_t address, uint8_t* values, uint8_t w_length);

#endif