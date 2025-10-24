#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H
#include "stm32l4xx.h"
#include "flash.h"

#define CONFIG_PAGE             127
#define CONFIG_SIZE_64          55  // размер конфига в словах (8 байт)
#define JSON_STR_CONFIG_SIZE    2800
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
        char apn[20];   // 4
        char ip[17];
        uint16_t port;
        uint8_t res;
        uint8_t action_mode;
        uint16_t module_id;
        uint8_t auto_save_config;
        uint8_t immediate_applying;
        uint8_t enable_beep;
        uint8_t enable_watchdog;
        uint8_t modem_period;

        uint32_t wakeup_period;  // 1
        uint32_t uart_speed;
        int32_t rtc_ppm;  // 2

        uint32_t lora_freq;
        uint8_t lora_sf;  // 3
        uint8_t lora_bw;
        uint8_t lora_cr;
        uint8_t lora_crc_en;
        uint8_t lora_ldro;
        uint8_t lora_sync_word;
        uint8_t lora_tx_power;
        uint8_t lora_preamble;


        uint64_t sensors_serials[45];  // 9
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
extern char config_json[JSON_STR_CONFIG_SIZE];

#endif