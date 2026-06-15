#include "LoRa.h"
#include <math.h>

#include "sx127x.h"
#include "sx126x.h"


#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

uint16_t Calc_TOA(uint8_t payload_size, uint8_t preamble_size,
                  uint8_t sf, float bw_kHz, uint8_t cr,
                  uint8_t explicit_header, uint8_t crc, uint8_t ldro) {
    float symbol_time_ms = (1 << sf) / bw_kHz;
    float preamble_time_ms = (preamble_size + 4.25) * symbol_time_ms;
    float tmp_poly = MAX(ceil(8 * payload_size - 4 * sf + 28 + 16 * crc - 20 * explicit_header), 0);
    float payload_symbol_nb = 8 + (tmp_poly / (4 * (sf - 2 * ldro))) * (4 + cr);  // Сколько символов занимает основная часть в пакете.
    return (uint16_t)(payload_symbol_nb * symbol_time_ms + preamble_time_ms);  // payload time + preamble_time
}

inline uint8_t LoRa_Init(uint32_t freq_hz, uint8_t sf, uint8_t bw, uint8_t cr, uint8_t power){
#ifdef USE_SX127x
    sx127x = (SX127x){
        .spi = LoRa_SPI,
        .bandWidth = system_config.lora_bw,
        .frequency = system_config.lora_freq,
        .power = system_config.lora_tx_power,
        .preamble = system_config.lora_preamble,
        .coding_rate = system_config.lora_cr,
        .spreadingFactor = system_config.lora_sf,
        .ldro = system_config.lora_ldro,
        .overCurrentProtection = 120,
        .new_rx_data_flag = 0,
        .delay = DWT_Delay_ms,
    };
    uint8_t result = SX127x_init(&SX1278);
    SX127x_gotoMode(&SX1278, RXCONTIN_MODE);
    return result;
#elif defined USE_SX126x
    SX1268 = (SX126x){
        .base = {
            .spi = LoRa_SPI,
            .config = {
                .frequency = freq_hz,
                .implicit_header = 0x00,
                .preamble = 8,
                .bandWidth = bw, // BW250
                .coding_rate = cr,  // CR4/5
                .crc_enable = 0x01,
                .iq_polarity = 0x00,
                .ldro = 0x01,
                .mode = 0x01, // LoRa
                .spreadingFactor = sf,
                .power_dbm = power,
                // .ramping_time = 0x03,
                .sync_word = 0x1424, // 0x12 (0x3444 = 0x34)
            },
            .rx_data = {},
            .tx_data = {},
        }
    };
    return SX126x_Init(&SX1268);
#endif
}

inline void LoRa_RxHandler(){
    #ifdef USE_SX127x
    SX127x_RxHandler(&SX1278);
    #elif defined USE_SX126x
    SX126x_RxHandler(&SX1268);
    #endif
}

inline void LoRa_Transmit(uint8_t *data, uint8_t length){
    #ifdef USE_SX127x
    SX127x_transmit(&SX1278, data, length);
    #elif defined USE_SX126x
    SX126x_SendData(&SX1268, data, length);
    #endif
}

