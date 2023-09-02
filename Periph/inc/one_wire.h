#ifndef INC_1_WIRE_H_
#define INC_1_WIRE_H_

#include "stm32l4xx.h"

#define OneWire_bit_1          0xFF
#define OneWire_bit_0          0x00
#define OneWire_read_bit       0xFF
#define OneWire_reset_cmd      0xF0

#define MAXDEVICES_ON_THE_BUS  20
#define OneWire_ROM_ID_lenth   64

typedef enum{
  ONE_WIRE_OK = 0,
  ONE_WIRE_EMPTY_BUS = !ONE_WIRE_OK
} OneWireStatus;

typedef union uRomCode{
    uint64_t serial_code;
    struct {
        uint8_t family;
        uint8_t code[6];
        uint8_t crc;
    };
} RomCode;

typedef struct OneWire{
    USART_TypeDef *uart;
    RomCode ids[MAXDEVICES_ON_THE_BUS];
    uint8_t crc_status[MAXDEVICES_ON_THE_BUS];
    int last_diff_bit_position;
    uint8_t lastROM[8];  // 64 bit ROM
} OneWire;

OneWireStatus OneWire_Reset(OneWire *ow);
uint8_t OneWire_ReadBit(OneWire *ow);
uint8_t OneWire_SendBit(OneWire *ow, uint8_t data);
uint8_t OneWire_Read(OneWire *ow);
uint8_t OneWire_Write(OneWire *ow, uint8_t byte);
void OneWire_ReadArray(OneWire *ow, uint8_t *array, uint8_t length);
void OneWire_WriteArray(OneWire *ow, uint8_t *array, uint8_t length);
int OneWire_SearchDevices(OneWire *ow);
OneWireStatus OneWire_MatchRom(OneWire *ow, RomCode *rom);
#endif /* INC_1_WIRE_H_ */
