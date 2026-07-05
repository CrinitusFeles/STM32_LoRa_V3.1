#include "radio_protocol.h"

uint16_t crc16_calc(uint8_t *buffer, uint16_t len){
   uint16_t fcs = 0xFFFF;
   while (len--)
      fcs = (fcs >> 8) ^ crc16_ccitt_table_reverse[(fcs ^ *buffer++) & 0xFF];
   return fcs ^ 0xFFFF;  // bit reversed
}