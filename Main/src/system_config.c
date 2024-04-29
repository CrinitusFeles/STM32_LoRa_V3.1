
#include "system_config.h"
#include "flash.h"
#include "string.h"
#include "system_select.h"


FLASH_status save_system_config(SystemConfig *config){
    uint8_t need_rewrite = 0;
    FLASH_status status = FLASH_OK;
    for(uint8_t i = 0; i < 16; i++){
        if(config->FLASH_page_buffer[i] != M64(config->config_addr + i * sizeof(uint64_t)))
            need_rewrite = 1;
    }
    if(need_rewrite){
        status = FLASH_erase_page(config->config_page);
        if(status != FLASH_OK) return status;
        status = SetPrefferedBlockNum(config->pref_block);
        if(status != FLASH_OK) return status;
        status = FLASH_write(config->config_addr, config->FLASH_page_buffer, 16);
    }
    return status;
}

int8_t is_FLASH_config_correct(SystemConfig *config){
    SystemConfig new_config;
    if(M64(config->config_addr) != FLASH_EMPTY_CEIL){
        for(uint8_t i = 0; i < 4; i++){
            new_config.FLASH_page_buffer[i] = M64(config->config_addr + i * sizeof(uint64_t));
        }
    } else {
        return -1;
    }
    if(new_config.config_addr < 0x8008000 || new_config.config_addr > 0x8038000){
        return -3;
    }
    if(new_config.serials_addr < 0x8008000 || new_config.serials_addr > 0x8038000){
        return -2;
    }
    if(new_config.config_page < 32 || new_config.config_page > 253){
        return -4;
    }
    if(new_config.uart_speed < 4800 || new_config.uart_speed > 115200){
        return -5;
    }
    if(new_config.lora_freq < 400000000 || new_config.lora_freq > 501000000){
        return -6;
    }
    if(new_config.lora_bw < 3 || new_config.lora_bw > 9){
        return -7;
    }
    if(new_config.wakeup_period < 1){
        return -8;
    }
    if(new_config.action_mode > 1){
        return -9;
    }
    return 0;
}

void system_config_init(SystemConfig *config){
    config->config_addr = 0x801F800;
    config->serials_addr = 0x801F820;
    config->config_page = 63;
    config->action_mode = 0;
    config->module_id = 0;
    config->wakeup_period = 20 * 60;
    config->uart_speed = 76800;
    config->lora_freq = 435000000;
    config->lora_sf = 10;
    config->lora_bw = 7;
    config->lora_cr = 1;
    config->lora_crc_en = 1;
    config->lora_ldro = 0;
    config->lora_sync_word = 0x12;
    config->lora_tx_power = 0xF6;
    config->lora_preamble = 8;
    memset(config->sensors_serials, 0xFF, 12 * sizeof(uint64_t));
}

int8_t write_system_config(SystemConfig *config){
    int8_t result = is_FLASH_config_correct(config);
    if(result < 0){
        if(save_system_config(config) != FLASH_OK) return -10;
        return 1;
    } else {  // read config from FLASH
        for(uint16_t i = 0; i < 16; i++){
            config->FLASH_page_buffer[i] = *(uint64_t *)(config->config_addr + i * sizeof(uint64_t));
        }
        return 0;
    }
}