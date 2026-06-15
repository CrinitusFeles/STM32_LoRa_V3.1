#ifndef INC_LORA_H
#define INC_LORA_H

#include "radio_protocol.h"
#include "main.h"

typedef struct LoRa_Config{
	uint8_t mode;
	uint32_t frequency;
	uint8_t	spreadingFactor;
	uint8_t	bandWidth;
    uint8_t implicit_header;
	uint8_t	coding_rate;
	uint16_t preamble;
	uint8_t	power_dbm;
	uint8_t	crc_enable;
	uint8_t	overCurrentProtection;
    uint8_t ldro;
    uint8_t iq_polarity;
    uint16_t sync_word;
} LoRa_Config;

typedef struct LoRa_s{
	SPI_TypeDef* spi;
    LoRa_Config config;
	// Module settings:
    uint8_t new_rx_data_flag;
    uint8_t rssi;
    int8_t snr;
    uint8_t rx_pkt_len;
    uint8_t rx_buf_ptr;
    uint8_t busy_issues;
    RadioProtocol rx_data;
    RadioProtocol tx_data;
    uint8_t transmitting_progress;
    void (*restart_watchdog)(void);
    void (*delay)(uint32_t);
} LoRa_t;



uint16_t Calc_TOA(uint8_t payload_size, uint8_t preamble_size,
                  uint8_t sf, float bw_kHz, uint8_t cr,
                  uint8_t explicit_header, uint8_t crc, uint8_t ldro);
void LoRa_Transmit(uint8_t *data, uint8_t length);
uint8_t LoRa_Init(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr, uint8_t power);
void LoRa_RxHandler();


#endif
