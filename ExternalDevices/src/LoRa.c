#include "LoRa.h"
#include <math.h>
#include "dwt.h"
#ifdef USE_SX126x
    #include "sx126x.h"
#elif defined USE_SX127x
    #include "sx127x.h"
#endif

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))

#ifdef USE_SX126x
SX126x SX1268;
#elif defined USE_SX127x
SX127x SX1278;
#endif

LoRa_t LoRa;

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
    LoRa = (LoRa_t){
        #ifdef USE_SX127x
        .driver_name = "SX1278",
        .delay = DWT_Delay_ms,
        #elif defined USE_SX126x
        .driver_name = "SX1268",
        .restart_watchdog = IWDG_refresh,
        #endif
        .spi = LoRa_SPI,
        .config = {
            .bandWidth = bw,
            .frequency = freq_hz,
            .power_dbm = power,
            .preamble = 8,
            .coding_rate = cr,
            .spreadingFactor = sf,
            .ldro = 1,
            .crc_enable = 1,
            .iq_polarity = 0,
            .mode = 1,
            .implicit_header = 0,
            .overCurrentProtection = 120,
            #ifdef USE_SX127x
            .sync_word = 0x12,
            #elif defined USE_SX126x
            .sync_word = 0x1424, // 0x12 (0x3444 = 0x34)
            #endif
        },

    };
    #ifdef USE_SX127x
    SX1278 = (SX127x){
        .base = &LoRa
    };
    uint8_t result = SX127x_init(&SX1278);
    SX127x_gotoMode(&SX1278, RXCONTIN_MODE);
    return result;
    #elif defined USE_SX126x
    SX1268 = (SX126x){
        .base = &LoRa
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

