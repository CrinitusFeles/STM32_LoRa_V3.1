
#include "system_config.h"
#include "flash.h"
#include "string.h"
#include "system_select.h"
#include "xprintf.h"
#include "json.h"
#include "ff.h"
#include "ds18b20_bus.h"

#define  NUM_FIELD_AMOUNT   22

FLASH_status save_system_config_to_FLASH(SystemConfig *config){
    /*
    1) чтение конфига из FLASH по адресу из переданного конфига
    2) если переданный конфиг не совпадает с конфигом на FLASH, то
    страница FLASH очищается и записывается новый конфиг
    */
    uint8_t need_rewrite = 0;
    FLASH_status status = FLASH_OK;
    uint32_t config_addr = FLASH_START_ADDR + CONFIG_PAGE * FLASH_PAGE_SIZE;
    for(uint8_t i = 0; i < CONFIG_SIZE_64; i++){
        if(config->FLASH_page_buffer[i] != M64(config_addr + i * sizeof(uint64_t))){
            need_rewrite = 1;
            break;
        }
    }
    config->pref_block = HavePrefFlashBlockNum();
    if(need_rewrite){
        status = FLASH_erase_page(CONFIG_PAGE);
        if(status != FLASH_OK) return status;
        status = SetPrefferedBlockNum(config->pref_block);
        if(status != FLASH_OK) return status;
        status = FLASH_write(config_addr, config->FLASH_page_buffer, CONFIG_SIZE_64);
    }
    return status;
}


SystemConfigStatus validate_config(SystemConfig *config){
    if(config->uart_speed < 4800 || config->uart_speed > 115200){
        return CONFIG_VALIDATION_ERROR;
    }
    if(config->lora_freq < 400000000 || config->lora_freq > 501000000){
        return CONFIG_VALIDATION_ERROR;
    }
    if(config->lora_bw < 3 || config->lora_bw > 9){
        return CONFIG_VALIDATION_ERROR;
    }
    if(config->wakeup_period < 1){
        return CONFIG_VALIDATION_ERROR;
    }
    if(config->action_mode > 1){
        return CONFIG_VALIDATION_ERROR;
    }
    return CONFIG_OK;
}

void system_config_init(SystemConfig *config){
    config->action_mode = 0;
    config->module_id = 0;
    config->auto_save_config = 0;
    config->immediate_applying = 0;
    config->enable_beep = 1;
    config->enable_watchdog = 1;
    config->wakeup_period = 20 * 60;
    config->uart_speed = 76800;
    config->rtc_ppm = -300;
    config->lora_freq = 435000000;
    config->lora_sf = 7;
    config->lora_bw = 8;
    config->lora_cr = 1;
    config->lora_crc_en = 1;
    config->lora_ldro = 0;
    config->lora_sync_word = 0x12;
    config->lora_tx_power = 0xF6;
    config->lora_preamble = 8;
    config->modem_period = 0;
    config->port = 33012;
    config->lora_enable = 1;
    config->upload_firmware = 0;
    strcpy(config->apn, "internet.tele2.ru\0\0");
    strcpy(config->ip, "84.237.52.8\0\0\0\0");
    memset(config->sensors_serials, 0xFF, TEMP_SENSOR_AMOUNT * sizeof(uint64_t));
}

char *config_fields[] = {
    "port",                 // 1
    "action_mode",          // 2
    "module_id",            // 3
    "auto_save_config",     // 4
    "immediate_applying",   // 5
    "enable_beep",          // 6
    "enable_watchdog",      // 7
    "modem_period",         // 8
    "wakeup_period",        // 9
    "uart_speed",           // 10
    "rtc_ppm",              // 11
    "lora_freq",            // 12
    "lora_sf",              // 13
    "lora_bw",              // 14
    "lora_cr",              // 15
    "lora_crc_en",          // 16
    "lora_ldro",            // 17
    "lora_sync_word",       // 18
    "lora_tx_power",        // 19
    "lora_preamble",        // 20
    "lora_enable",          // 21
    "upload_firmware"       // 22  NUM_FIELD_AMOUNT
};


uint16_t system_config_to_str(SystemConfig *config, char *buf){
    uint16_t written = xsprintf(buf,  "{\n");
    for(uint8_t i = 0; i < NUM_FIELD_AMOUNT; i++){
        written +=  xsprintf(buf + written,  "    \"%s\": %d,\n",
                             config_fields[i], *((int32_t *)(&config->port) + i));
    }
    written += xsprintf(buf + written,  "    \"apn\": \"%s\",\n", config->apn);
    written += xsprintf(buf + written,  "    \"ip\": \"%s\",\n", config->ip);
    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        written += xsprintf(buf + written, "    \"temp_sensor_id%d\": \"0x%llX\",\n",
                            i + 1, config->sensors_serials[i]);
    }
    xsprintf(buf + written - 3, "\n}\n");
    return written;
}

void parse_system_config(SystemConfig *config, char *buf, int buf_len){
    char str_buf[30] = "";
    long val = 0;
    for(uint8_t i = 0; i < NUM_FIELD_AMOUNT; i++){
        int written = xsprintf(str_buf, "$.%s", config_fields[i]);
        json_get_num(buf, buf_len, str_buf, &val);
        *((int32_t *)(&config->port) + i) = val;
        if(written > 0) memset(str_buf, 0, written);
    }
    json_get_str(buf, buf_len, "$.apn", (char *)&config->apn, 20);
    json_get_str(buf, buf_len, "$.ip", (char *)&config->ip, 17);
    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        uint64_t serial = 0;
        char hex_val[20] = {0};
        xsprintf(str_buf, "$.temp_sensor_id%d", i+1);
        json_get_str(buf, buf_len, str_buf, hex_val, 20);
        xatoll(hex_val, (long long *)(&serial));
        config->sensors_serials[i] = serial;
    }
}

SystemConfigStatus read_FLASH_system_config(SystemConfig *config){
    uint32_t config_addr = FLASH_START_ADDR + CONFIG_PAGE * FLASH_PAGE_SIZE;
    if(M64(config_addr) == FLASH_EMPTY_CELL){
        return CONFIG_FLASH_EMPTY;
    }

    for(uint16_t i = 0; i < CONFIG_SIZE_64; i++){  // read config from FLASH
        config->FLASH_page_buffer[i] = M64(config_addr + i * sizeof(uint64_t));
    }
    SystemConfigStatus status = validate_config(config);
    return status;
}

/*
Выполняется единожды при инициализации системы
*/
SystemConfigStatus init_FLASH_system_config(SystemConfig *config){
    if(M64(FLASH_START_ADDR + CONFIG_PAGE * FLASH_PAGE_SIZE) == FLASH_EMPTY_CELL){
        if(save_system_config_to_FLASH(config) != FLASH_OK)
            return CONFIG_SAVE_ERROR;
        return CONFIG_OK;
    }
    return read_FLASH_system_config(config);
}


SystemConfigStatus save_config_to_SD(SystemConfig *config, char *path,
                                     char *json){
    UINT written_count = 0;
    UINT read_count = 0;
    uint16_t json_len = 0;

    if(validate_config(config) != CONFIG_OK){
        return CONFIG_VALIDATION_ERROR;
    }
    json_len = system_config_to_str(config, json);
    if(f_open(&file, path, FA_OPEN_ALWAYS | FA_WRITE | FA_READ) != FR_OK)
        return CONFIG_SD_ERROR;
    if(f_read(&file, file_buff, JSON_STR_CONFIG_SIZE, &read_count) != FR_OK){
        f_close(&file);
        return CONFIG_SD_ERROR;
    }
    if(read_count == 0 || memcmp(file_buff, json, json_len)){
        f_lseek(&file, 0);
        if(f_write(&file, json, json_len, &written_count) != FR_OK){
            f_close(&file);
            return CONFIG_SD_ERROR;
        }
        f_close(&file);
        if(written_count != json_len){
            return CONFIG_SD_ERROR;
        }
    }
    f_close(&file);
    return CONFIG_OK;
}

SystemConfigStatus read_config_from_SD(SystemConfig *config, char *path,
                                       char *json, uint16_t json_size){
    UINT read_count = 0;
    if(f_open(&file, path, FA_OPEN_ALWAYS | FA_READ) != FR_OK)
        return CONFIG_SD_ERROR;
    if(f_read(&file, json, json_size, &read_count) != FR_OK){
        f_close(&file);
        return CONFIG_SD_ERROR;
    }
    if(read_count < 10){
        f_close(&file);
        return CONFIG_SD_EMPTY;
    }
    f_close(&file);
    parse_system_config(config, json, read_count);
    if(validate_config(config) != CONFIG_OK){
        return CONFIG_VALIDATION_ERROR;
    }
    return CONFIG_OK;
}
