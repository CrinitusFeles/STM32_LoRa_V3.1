/**
 * @file    xmodem.h
 * @author  Ferenc Nemeth
 * @date    21 Dec 2018
 * @brief   This module is the implementation of the Xmodem protocol.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */

#ifndef XMODEM_H_
#define XMODEM_H_

#include "stdint.h"
#include "stdbool.h"

/* Xmodem (128 bytes) packet format
 * Byte  0:       Header
 * Byte  1:       Packet number
 * Byte  2:       Packet number complement
 * Bytes 3-130:   Data
 * Bytes 131-132: CRC
 */

/* Xmodem (1024 bytes) packet format
 * Byte  0:         Header
 * Byte  1:         Packet number
 * Byte  2:         Packet number complement
 * Bytes 3-1026:    Data
 * Bytes 1027-1028: CRC
 */

/* Maximum allowed errors (user defined). */
#define X_MAX_ERRORS          ((uint8_t)3)
#define X_HEADER_TIMEOUT_MS     1000

/* Sizes of the packets. */
#define X_PACKET_NUMBER_SIZE  ((uint16_t)2)
#define X_PACKET_128_SIZE     ((uint16_t)128)
#define X_PACKET_1024_SIZE    ((uint16_t)1024)
#define X_PACKET_CRC_SIZE     ((uint16_t)2)

/* Indexes inside packets. */
#define X_PACKET_NUMBER_INDEX             ((uint16_t)0)
#define X_PACKET_NUMBER_COMPLEMENT_INDEX  ((uint16_t)1)
#define X_PACKET_CRC_HIGH_INDEX           ((uint16_t)0)
#define X_PACKET_CRC_LOW_INDEX            ((uint16_t)1)
#define _X_UART USART1
#ifndef _X_UART
#error "You need to define X_UART"
#endif


/* Bytes defined by the protocol. */
#define X_SOH ((uint8_t)0x01)  /**< Start Of Header (128 bytes). */
#define X_STX ((uint8_t)0x02)  /**< Start Of Header (1024 bytes). */
#define X_EOT ((uint8_t)0x04)  /**< End Of Transmission. */
#define X_ACK ((uint8_t)0x06)  /**< Acknowledge. */
#define X_NAK ((uint8_t)0x15)  /**< Not Acknowledge. */
#define X_CAN ((uint8_t)0x18)  /**< Cancel. */
#define X_C   ((uint8_t)0x43)  /**< ASCII "C" to notify the host we want to use CRC16. */

#define CTRL_C ((uint8_t)0x03)

/* Status report for the functions. */
typedef enum {
  X_OK            = 0x00, /**< The action was successful. */
  X_ERROR_CRC     = 0x01, /**< CRC calculation error. */
  X_ERROR_NUMBER  = 0x02, /**< Packet number mismatch error. */
  X_ERROR_UART    = 0x04, /**< UART communication error. */
  X_ERROR_FLASH   = 0x08, /**< Flash related error. */
  X_ERROR         = 0xFF  /**< Generic error. */
} xmodem_status;



typedef struct XModem{
    void (*delay)(uint32_t ms);
    bool (*save)(uint32_t addr, uint8_t *buff, uint32_t size);
    bool (*on_first_packet)(void);
} XModem;


void xmodem_receive(XModem *xmodem, uint32_t write_addr);

extern XModem xmodem;

#endif /* XMODEM_H_ */