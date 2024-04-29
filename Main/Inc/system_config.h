#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H
#include "stm32l4xx.h"
#include "flash.h"

typedef union SystemConfig{
    uint64_t FLASH_page_buffer[256];
    struct{
        uint32_t config_addr;
        uint32_t serials_addr;
        uint8_t config_page;
        uint8_t action_mode;
        uint16_t module_id;
        uint8_t auto_save_config;

        uint32_t wakeup_period;
        uint32_t uart_speed;

        uint32_t lora_freq;
        uint8_t lora_sf;
        uint8_t lora_bw;
        uint8_t lora_cr;
        uint8_t lora_crc_en;
        uint8_t lora_ldro;
        uint8_t lora_sync_word;
        uint8_t lora_tx_power;
        uint8_t lora_preamble;

        uint64_t sensors_serials[12];
        uint64_t pref_block;
    };
} SystemConfig;

FLASH_status save_system_config(SystemConfig *config);
void system_config_init(SystemConfig *config);
int8_t write_system_config(SystemConfig *config);
extern SystemConfig system_config;

#endif