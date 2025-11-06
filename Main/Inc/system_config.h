#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H
#include "stm32l4xx.h"
#include "flash.h"
#include "ff.h"

#define CONFIG_PAGE             127
#define CONFIG_SIZE_64          62  // размер конфига в словах (8 байт)
#define JSON_STR_CONFIG_SIZE    2800
#define SYSTEM_CONFIG_PATH      "system_config.json"
#define FILE_BUFFER             4096

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
        char apn[20];
        char ip[20];
        int32_t port;  // 5
        int32_t action_mode;  //6
        int32_t module_id;
        int32_t auto_save_config;  // 7
        int32_t immediate_applying;
        int32_t enable_beep;  // 8
        int32_t enable_watchdog;
        int32_t modem_period;  // 9
        int32_t wakeup_period;
        int32_t uart_speed;  // 10
        int32_t rtc_ppm;
        int32_t lora_freq;  // 11
        int32_t lora_sf;
        int32_t lora_bw;  // 12
        int32_t lora_cr;
        int32_t lora_crc_en;  // 13
        int32_t lora_ldro;
        int32_t lora_sync_word;  // 14
        int32_t lora_tx_power;
        int32_t lora_preamble;  // 15
        int32_t lora_enable;
        int32_t upload_firmware;  // 16

        uint64_t sensors_serials[45];
        uint64_t pref_block;
    };
} SystemConfig;

FLASH_status save_system_config_to_FLASH(SystemConfig *config);
void system_config_init(SystemConfig *config);
uint16_t system_config_to_str(SystemConfig *config, char *buf);
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
extern char file_buff[FILE_BUFFER];
extern FIL file;
#endif