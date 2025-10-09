#ifndef RADIOPROTOCOL_H
#define RADIOPROTOCOL_H

#include "stm32l4xx.h"

typedef union RadioProtocol{
    uint8_t buffer[255];
    struct {
        uint8_t dst_addr;
        uint8_t src_addr;
        uint8_t dlen;
        uint16_t crc16 __attribute__((packed,aligned(1)));
        uint8_t payload[250];
    };
} RadioProtocol;

#endif