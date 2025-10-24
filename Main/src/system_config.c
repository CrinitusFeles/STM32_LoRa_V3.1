
#include "system_config.h"
#include "flash.h"
#include "string.h"
#include "system_select.h"
#include "xprintf.h"
#include "json.h"
#include "ff.h"
#include "ds18b20_bus.h"



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
    strcpy(config->apn, "internet.tele2.ru\0\0");
    strcpy(config->ip, "84.237.52.8\0\0\0\0");
    memset(config->sensors_serials, 0xFF, TEMP_SENSOR_AMOUNT * sizeof(uint64_t));
}

void system_config_to_str(SystemConfig *config, char *buf){
    uint16_t written = xsprintf(buf,
    "{\n"\
    "    \"action_mode\": %d,\n"\
    "    \"module_id\": %d,\n"\
    "    \"auto_save_config\": %d,\n"\
    "    \"immediate_applying\": %d,\n"\
    "    \"enable_beep\": %d,\n"\
    "    \"enable_watchdog\": %d,\n"\
    "    \"wakeup_period\": %d,\n"\
    "    \"uart_speed\": %d,\n"\
    "    \"rtc_ppm\": %d,\n"\
    "    \"lora_freq\": %d,\n"\
    "    \"lora_sf\": %d,\n"\
    "    \"lora_bw\": %d,\n"\
    "    \"lora_cr\": %d,\n"\
    "    \"lora_crc_en\": %d,\n"\
    "    \"lora_ldro\": %d,\n"\
    "    \"lora_sync_word\": %d,\n"\
    "    \"lora_tx_power\": %d,\n"\
    "    \"lora_preamble\": %d,\n"\
    "    \"modem_period\": %d,\n"\
    "    \"apn\": %s,\n"\
    "    \"ip\": %s,\n"\
    "    \"port\": %d,\n",
    config->action_mode,
    config->module_id,
    config->auto_save_config,
    config->immediate_applying,
    config->enable_beep,
    config->enable_watchdog,
    config->wakeup_period,
    config->uart_speed,
    config->rtc_ppm,
    config->lora_freq,
    config->lora_sf,
    config->lora_bw,
    config->lora_cr,
    config->lora_crc_en,
    config->lora_ldro,
    config->lora_sync_word,
    config->lora_tx_power,
    config->lora_preamble,
    config->modem_period,
    config->apn,
    config->ip,
    config->port
    );
    for(uint8_t i = 0; i < 45; i++){
        written += xsprintf(buf + written, "    \"temp_sensor_id%d\": %llu,\n",
                           i + 1, config->sensors_serials[i]);
    }
    xsprintf(buf + written, "}\n");
}

void parse_system_config(SystemConfig *config, char *buf, int buf_len){
    char str_buf[30] = "";
    json_get_num(buf, buf_len, "$.action_mode", (long *)&config->action_mode);
    json_get_num(buf, buf_len, "$.module_id", (long *)&config->module_id);
    json_get_num(buf, buf_len, "$.auto_save_config", (long *)&config->auto_save_config);
    json_get_num(buf, buf_len, "$.immediate_applying", (long *)&config->immediate_applying);
    json_get_num(buf, buf_len, "$.enable_beep", (long *)&config->enable_beep);
    json_get_num(buf, buf_len, "$.enable_watchdog", (long *)&config->enable_watchdog);
    json_get_num(buf, buf_len, "$.wakeup_period", (long *)&config->wakeup_period);
    json_get_num(buf, buf_len, "$.uart_speed", (long *)&config->uart_speed);
    json_get_num(buf, buf_len, "$.rtc_ppm", (long *)&config->rtc_ppm);
    json_get_num(buf, buf_len, "$.lora_freq", (long *)&config->lora_freq);
    json_get_num(buf, buf_len, "$.lora_sf", (long *)&config->lora_sf);
    json_get_num(buf, buf_len, "$.lora_bw", (long *)&config->lora_bw);
    json_get_num(buf, buf_len, "$.lora_cr", (long *)&config->lora_cr);
    json_get_num(buf, buf_len, "$.lora_crc_en", (long *)&config->lora_crc_en);
    json_get_num(buf, buf_len, "$.lora_ldro", (long *)&config->lora_ldro);
    json_get_num(buf, buf_len, "$.lora_sync_word", (long *)&config->lora_sync_word);
    json_get_num(buf, buf_len, "$.lora_tx_power", (long *)&config->lora_tx_power);
    json_get_num(buf, buf_len, "$.lora_preamble", (long *)&config->lora_preamble);
    json_get_num(buf, buf_len, "$.port", (long *)&config->port);
    json_get_num(buf, buf_len, "$.modem_period", (long *)&config->modem_period);
    json_get_str(buf, buf_len, "$.apn", (char *)&config->apn, 20);
    json_get_str(buf, buf_len, "$.ip", (char *)&config->ip, 17);
    for(uint8_t i = 0; i < 45; i++){
        xsprintf(str_buf, "$.temp_sensor_id%d", i+1);
        json_get_big_num(buf, buf_len, str_buf, (long long*)&config->sensors_serials[i]);
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
    FIL file;
    UINT written_count = 0;
    UINT read_count = 0;
    uint16_t json_len;
    char json_old[JSON_STR_CONFIG_SIZE] = {0};

    if(validate_config(config) != CONFIG_OK){
        return CONFIG_VALIDATION_ERROR;
    }
    system_config_to_str(config, json);
    json_len = strlen(json);
    if(f_open(&file, path, FA_OPEN_ALWAYS | FA_WRITE | FA_READ) != FR_OK)
        return CONFIG_SD_ERROR;
    if(f_read(&file, json_old, JSON_STR_CONFIG_SIZE, &read_count) != FR_OK){
        f_close(&file);
        return CONFIG_SD_ERROR;
    }
    if(memcmp(json_old, json, json_len)){
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
    FIL file;
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
