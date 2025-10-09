
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
    for(uint8_t i = 0; i < CONFIG_SIZE_64; i++){
        if(config->FLASH_page_buffer[i] != M64(config->config_addr + i * sizeof(uint64_t))){
            need_rewrite = 1;
            break;
        }
    }
    config->pref_block = HavePrefFlashBlockNum();
    if(need_rewrite){
        status = FLASH_erase_page(config->config_page);
        if(status != FLASH_OK) return status;
        status = SetPrefferedBlockNum(config->pref_block);
        if(status != FLASH_OK) return status;
        status = FLASH_write(config->config_addr, config->FLASH_page_buffer, CONFIG_SIZE_64);
    }
    return status;
}

uint8_t is_FLASH_config_empty(SystemConfig *config){
    for(uint8_t i = 0; i < CONFIG_SIZE_64 - 12; i++){
        if(M64(config->config_addr + i * sizeof(uint64_t)) != FLASH_EMPTY_CELL){
            return 0;
        }
    }
    return 1;
}

SystemConfigStatus validate_config(SystemConfig *config){
    if(config->config_addr < 0x8008000 || config->config_addr > 0x8038000){
        return CONFIG_VALIDATION_ERROR;
    }
    if(config->serials_addr < 0x8008000 || config->serials_addr > 0x8038000){
        return CONFIG_VALIDATION_ERROR;
    }
    if(config->config_page < 32 || config->config_page > 253){
        return CONFIG_VALIDATION_ERROR;
    }
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
    config->config_addr = 0x801F800;
    config->serials_addr = 0x801F820;
    config->config_page = 63;
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
    memset(config->sensors_serials, 0xFF, TEMP_SENSOR_AMOUNT * sizeof(uint64_t));
}

void system_config_to_str(SystemConfig *config, char *buf){
    xsprintf(buf,
    "{\n"\
    "    \"config_addr\": %d,\n"\
    "    \"serials_addr\": %d,\n"\
    "    \"config_page\": %d,\n"\
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
    "    \"temp_sensor_id1\": %llu,\n"\
    "    \"temp_sensor_id2\": %llu,\n"\
    "    \"temp_sensor_id3\": %llu,\n"\
    "    \"temp_sensor_id4\": %llu,\n"\
    "    \"temp_sensor_id5\": %llu,\n"\
    "    \"temp_sensor_id6\": %llu,\n"\
    "    \"temp_sensor_id7\": %llu,\n"\
    "    \"temp_sensor_id8\": %llu,\n"\
    "    \"temp_sensor_id9\": %llu,\n"\
    "    \"temp_sensor_id10\": %llu,\n"\
    "    \"temp_sensor_id11\": %llu,\n"\
    "    \"temp_sensor_id12\": %llu,\n"\
    "    \"temp_sensor_id13\": %llu,\n"\
    "    \"temp_sensor_id14\": %llu,\n"\
    "    \"temp_sensor_id15\": %llu,\n"\
    "    \"temp_sensor_id16\": %llu,\n"\
    "    \"temp_sensor_id17\": %llu,\n"\
    "    \"temp_sensor_id18\": %llu,\n"\
    "    \"temp_sensor_id19\": %llu,\n"\
    "    \"temp_sensor_id20\": %llu,\n"\
    "    \"temp_sensor_id21\": %llu,\n"\
    "    \"temp_sensor_id22\": %llu,\n"\
    "    \"temp_sensor_id23\": %llu,\n"\
    "    \"temp_sensor_id24\": %llu,\n"\
    "    \"temp_sensor_id25\": %llu,\n"\
    "    \"temp_sensor_id26\": %llu,\n"\
    "    \"temp_sensor_id27\": %llu,\n"\
    "    \"temp_sensor_id28\": %llu,\n"\
    "    \"temp_sensor_id29\": %llu,\n"\
    "    \"temp_sensor_id30\": %llu,\n"\
    "    \"temp_sensor_id31\": %llu,\n"\
    "    \"temp_sensor_id32\": %llu,\n"\
    "    \"temp_sensor_id33\": %llu,\n"\
    "    \"temp_sensor_id34\": %llu,\n"\
    "    \"temp_sensor_id35\": %llu,\n"\
    "    \"temp_sensor_id36\": %llu,\n"\
    "    \"temp_sensor_id37\": %llu,\n"\
    "    \"temp_sensor_id38\": %llu,\n"\
    "    \"temp_sensor_id39\": %llu,\n"\
    "    \"temp_sensor_id40\": %llu,\n"\
    "    \"temp_sensor_id41\": %llu,\n"\
    "    \"temp_sensor_id42\": %llu,\n"\
    "    \"temp_sensor_id43\": %llu,\n"\
    "    \"temp_sensor_id44\": %llu,\n"\
    "    \"temp_sensor_id45\": %llu\n"\
    "}\n",
    config->config_addr,
    config->serials_addr,
    config->config_page,
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
    config->sensors_serials[0],
    config->sensors_serials[1],
    config->sensors_serials[2],
    config->sensors_serials[3],
    config->sensors_serials[4],
    config->sensors_serials[5],
    config->sensors_serials[6],
    config->sensors_serials[7],
    config->sensors_serials[8],
    config->sensors_serials[9],
    config->sensors_serials[10],
    config->sensors_serials[11],
    config->sensors_serials[12],
    config->sensors_serials[13],
    config->sensors_serials[14],
    config->sensors_serials[15],
    config->sensors_serials[16],
    config->sensors_serials[17],
    config->sensors_serials[18],
    config->sensors_serials[19],
    config->sensors_serials[20],
    config->sensors_serials[21],
    config->sensors_serials[22],
    config->sensors_serials[23],
    config->sensors_serials[24],
    config->sensors_serials[25],
    config->sensors_serials[26],
    config->sensors_serials[27],
    config->sensors_serials[28],
    config->sensors_serials[29],
    config->sensors_serials[30],
    config->sensors_serials[31],
    config->sensors_serials[32],
    config->sensors_serials[33],
    config->sensors_serials[34],
    config->sensors_serials[35],
    config->sensors_serials[36],
    config->sensors_serials[37],
    config->sensors_serials[38],
    config->sensors_serials[39],
    config->sensors_serials[40],
    config->sensors_serials[41],
    config->sensors_serials[42],
    config->sensors_serials[43],
    config->sensors_serials[44],
    config->sensors_serials[45]
    );
}

void parse_system_config(SystemConfig *config, char *buf, int buf_len){
    char str_buf[30] = "";
    json_get_num(buf, buf_len, "$.config_addr", (long *)&config->config_addr);
    json_get_num(buf, buf_len, "$.serials_addr", (long *)&config->serials_addr);
    json_get_num(buf, buf_len, "$.config_page", (long *)&config->config_page);
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
    for(uint8_t i = 0; i < 45; i++){
        xsprintf(str_buf, "$.temp_sensor_id%d", i+1);
        json_get_big_num(buf, buf_len, str_buf, (long long*)&config->sensors_serials[i]);
    }
}

SystemConfigStatus read_FLASH_system_config(SystemConfig *config){
    if(is_FLASH_config_empty(config)){
        return CONFIG_FLASH_EMPTY;
    }

    for(uint16_t i = 0; i < CONFIG_SIZE_64; i++){  // read config from FLASH
        config->FLASH_page_buffer[i] = M64(config->config_addr + i * sizeof(uint64_t));
    }
    SystemConfigStatus status = validate_config(config);
    return status;
}

/*
Выполняется единожды при инициализации системы
*/
SystemConfigStatus init_FLASH_system_config(SystemConfig *config){
    if(is_FLASH_config_empty(config)){
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
    return CONFIG_OK;
}
