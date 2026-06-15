#ifndef SX126x_MISC_H
#define SX126x_MISC_H
#include "gpio.h"
#include "sx126x.h"


void SX126x_SendOpcode(SX126x *driver, uint8_t opcode, uint8_t *parameters,
                       uint8_t param_len, uint8_t *answer, uint8_t answer_len);

#endif