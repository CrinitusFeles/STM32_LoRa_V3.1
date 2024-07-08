#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H
#include "stm32l4xx.h"
#include "flash.h"

#define CONFIG_SIZE_64          17  // размер конфига в словах (8 байт)
#define JSON_CONFIG_SIZE        512
#define SYSTEM_CONFIG_PATH      "system_config.json"

typedef enum SystemConfigStatus{
    CONFIG_OK,
    CONFIG_VALIDATION_ERROR,
    CONFIG_FLASH_EMPTY,
    CONFIG_SAVE_ERROR,
    CONFIG_SD_EMPTY,
    CONFIG_SD_ERROR,
}SystemConfigStatus;

typedef union SystemConfig{
    uint64_t FLASH_page_buffer[256];
    struct{
        uint32_t config_addr;
        uint32_t serials_addr;
        uint8_t config_page;  // 1
        uint8_t action_mode;
        uint16_t module_id;
        uint8_t auto_save_config;
        uint8_t immediate_applying;
        uint8_t enable_beep;
        uint8_t enable_watchdog;

        uint32_t wakeup_period;  // 2
        uint32_t uart_speed;
        int32_t rtc_ppm;

        uint32_t lora_freq;  // 3
        uint8_t lora_sf;
        uint8_t lora_bw;
        uint8_t lora_cr;
        uint8_t lora_crc_en;  // 4
        uint8_t lora_ldro;
        uint8_t lora_sync_word;
        uint8_t lora_tx_power;
        uint8_t lora_preamble;
        uint64_t res;

        uint64_t sensors_serials[12];
        uint64_t pref_block;
    };
} SystemConfig;

FLASH_status save_system_config_to_FLASH(SystemConfig *config);
void system_config_init(SystemConfig *config);
void system_config_to_str(SystemConfig *config, char *buf);
SystemConfigStatus init_FLASH_system_config(SystemConfig *config);
SystemConfigStatus read_FLASH_system_config(SystemConfig *config);
SystemConfigStatus validate_config(SystemConfig *config);
void parse_system_config(SystemConfig *config, char *buf, int buf_len);
SystemConfigStatus save_config_to_SD(SystemConfig *config, char *path,
                                     char *json);
SystemConfigStatus read_config_from_SD(SystemConfig *config, char *path,
                                       char *json, uint16_t json_size);
extern SystemConfig system_config;
extern char config_json[JSON_CONFIG_SIZE];

#endif