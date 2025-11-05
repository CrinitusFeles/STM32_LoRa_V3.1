#include "stm32_misc.h"
#include <string.h>
#include "xprintf.h"
#include "main.h"
#include "config.h"
#include "buzzer.h"
#include "ff.h"
#include "flash.h"
#include "uart.h"
#include "monitor_task.h"
#include "rtc.h"
#include "sx127x.h"
#include "sx127x_misc.h"
#include "system_select.h"
#include "xmodem.h"
#include "ds18b20_bus.h"
#include "system_config.h"
#include "json.h"
#include "gsm.h"
#include "periph_handlers.h"
#include "adc.h"
#include "lua_misc.h"
#include "action_task.h"
#include "console_utils.h"
#include "sensors_task.h"

#define MOD(x, y) (x & (y - 1))  // y must be power of 2!


#define _STM_DEMO_VER "1.2.0"


#define _CMD_HELP            2090324718
#define _CMD_CLEAR           255552908
#define _CMD_ECHO            2090214596
#define _CMD_BEEP            2090108865
#define _CMD_RESET           273105544
#define _CMD_TASKS           275333835
#define _CMD_XMODEM          666702863
#define _CMD_PREF_BLOCK      2687071548
#define _CMD_CURR_BLOCK      1680347659
#define _CMD_ERASE_FIRMWARE  3540328785
#define _CMD_CHECK_CRC       1181315578
#define _CMD_SEND_RADIO      2003746781
#define _CMD_RADIO_CONF      197085785
#define _CMD_SENS_MEASURE    1772722127
#define _CMD_CALIB_SENSORS   1652519980
#define _CMD_SLEEP           274527774
#define _CMD_TIME            2090760340
#define _CMD_DUMP_MEM        4197134393
#define _CMD_MOUNT           267537784
#define _CMD_NEOFETCH        1762818289
#define _CMD_LS              5863588
#define _CMD_CD              5863276
#define _CMD_RM              5863780
#define _CMD_RENAME          422364189
#define _CMD_TOUCH           275838856
#define _CMD_MKDIR           267375356
#define _CMD_CAT             193488125
#define _CMD_TAIL            2090751503
#define _CMD_HEAD            2090324343
#define _CMD_DF              5863311
#define _CMD_SET_CONFIG      2302414790
#define _CMD_SHOW_CONFIG     1288376539
#define _CMD_SAVE_CONFIG     3276224553
#define _CMD_CLEAR_CONFIG    4200672097
#define _CMD_UPLOAD_SD_FW    3279612316
#define _CMD_WATCHDOG        2398021142
#define _CMD_TOGGLE_GSM      2531157517
#define _CMD_GSM_CMD         278497759
#define _CMD_TCP_INIT        1518441119
#define _CMD_TCP_OPEN        1518658781
#define _CMD_TCP_CLOSE       2856735873
#define _CMD_TCP_SEND        1518790837
#define _CMD_LUA             193498567
#define _CMD_RUN_ACTION      1038971383
#define _CMD_DUMP_FILE       1066234362


#define _NUM_OF_CMD 45


uint32_t hash(const char *str) {
    uint32_t hash = 5381;
    char c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}


void uart_print(int data){
    UART_tx(USART_PRINT, (uint8_t)(data));
}


// available  commands
char *keyword[] = {
    "help",                 //  1
    "clear",                //  2
    "echo",                 //  3
    "beep",                 //  4
    "reset",                //  5
    "tasks",                //  6
    "xmodem",               //  7
    "pref_block",           //  8
    "curr_block",           //  9
    "erase_firmware",       //  10
    "check_crc",            //  11
    "send_radio",           //  12
    "radio_conf",           //  13
    "sens_measure",         //  14
    "calib_sensors",        //  15
    "sleep",                //  16
    "time",                 //  17
    "dump_mem",             //  18
    "mount",                //  19
    "neofetch",             //  20
    "ls",                   //  21
    "cd",                   //  22
    "rm",                   //  23
    "rename",               //  24
    "touch",                //  25
    "mkdir",                //  26
    "cat",                  //  27
    "tail",                 //  28
    "head",                 //  29
    "df",                   //  30
    "set_config",           //  31
    "show_config",          //  32
    "save_config",          //  33
    "clear_config",         //  34
    "upload_sd_fw",         //  35
    "watchdog",             //  36
    "toggle_gsm",           //  37
    "gsm_cmd",              //  38
    "tcp_init",             //  39
    "tcp_open",             //  40
    "tcp_close",            //  41
    "tcp_send",             //  42
    "lua",                  //  43
    "run_action",           //  44
    "dump_file",            //  45
};

// array for comletion
char *compl_word[_NUM_OF_CMD + 1];


//*****************************************************************************
/*
need to implements:
    df
*/
void print_help(void) {
    xprintf("microrl v" MICRORL_LIB_VER " library STM32 DEMO v" _STM_DEMO_VER "\n"\
            "Use TAB key for completion\nCommand:\n"\
      /*2*/   "  clear                          - clear screen\n"\
      /*3*/   "  echo [data]                    - print arg to screen\n"\
      /*4*/   "  ls [path]                      - show directory content\n"\
      /*5*/   "  neofetch                       - show system info'\n"\
      /*6*/   "  cd [path]                      - change directory\n"\
      /*7*/   "  rm [dir_or_file_name]          - remove file or directory\n"\
      /*8*/   "  touch [file_name]              - create file\n"\
      /*9*/   "  rename [old_name] [new_name]   - rename/move file\n"\
      /*10*/  "  mkdir [dir_name]               - create directory\n"\
      /*11*/  "  cat [file_name]                - read/write file\n"\
      /*12*/  "  tail [file_name] [str_count]   - read last strings of file\n"\
      /*13*/  "  head [file_name] [str_count]   - read first strings of file\n"\
      /*14*/  "  reset                          - reset MCU\n"\
      /*15*///  "  df                             - show disk status\n"
      /*16*/  "  mount                          - mount SD card\n"\
      /*17*/  "  dump_mem [addr] [length]       - dump memory slice\n"\
      /*18*/  "  tasks                          - show tasks\n"\
      /*19*/  "  xmodem                         - start flashing procedure over XModem protocol\n"\
      /*20*/  "  pref_block [block_num]         - get/set firmware preffered block\n"\
      /*21*/  "  curr_block                     - get current firmware block\n"\
      /*22*/  "  erase_firmware                 - erase firmware \n"\
      /*27*/  "  upload_sd_fw [file_name]       - write firmware to FLASH from SD card\n"\
      /*23*/  "  set_config [key] [val]         - set value for current config\n"\
      /*23*/  "  save_config                    - save system configuration to FLASH and SD card\n"\
      /*24*/  "  show_config [mode]             - show default or FLASH config\n"
      /*25*/  "  clear_config                   - rewrite FLASH config by default\n"
      /*26*/  "  check_crc [block_num]          - check firmware CRC \n"\
      /*28*/  "  sens_measure [mode] [file_path] [nums] [t_period] - measure temperature and soil moisture sensors\n"\
      /*30*/  "  calib_sensors                  - start temperature sensors calibration routine\n"\
      /*33*/  "  send_radio [*data]             - send LoRa package\n"\
      /*34*/  "  radio_conf [sf] [bw] [cr] [crc_en] [ldro] [tx_pow] - show/set LoRa configuration\n"\
      /*35*/  "  time [year] [month] [day] [hours] [mins] [seconds] - get/set MCU RTC\n"\
      /*36*/  "  beep [freq] [duration]         - make beep signal certain frequency and duration\n"\
      /*37*/  "  sleep [sleep_sec]              - go to sleep mode\n"
      /*37*/  "  watchdog                       - test watchdog timer\n"
      /*38*/  "  toggle_gsm                     - toggle GSM power\n"
      /*39*/  "  gsm_cmd                        - send command to GSM modem\n"
      );
}


bool xmodem_flash_write(uint32_t addr, uint8_t *buff, uint32_t size){
    FLASH_status status = FLASH_write(addr, (uint64_t*)&buff[0], (uint32_t)size / 8);
    if(status == FLASH_OK)
        return true;
    return false;
}

bool xmodem_sd_write(uint32_t addr, uint8_t *buff, uint32_t size){
    static size_t written_amount = 0;
    FIL file;
    FRESULT res = f_lseek(&file, addr);
    if(res != FR_OK){
        f_close(&file);
        xprintf("Failed seek file. Written %d bytes\n", written_amount);
        return false;
    }
    res = f_write(&file, buff, size, &written_amount);
    if(res == FR_OK)
        return true;
    return false;
}

bool xmodem_sd_on_first_packet(){
    return true;
}



char curr_dir_name[128] = "";
char rtc_buffer[25] = {0};
Args args;
//*****************************************************************************
// execute callback for microrl library
// do what you want here, but don't write to argv!!! read only!!
int execute(int argc, const char *const *argv) {
    switch (hash(argv[0]))
    {
    case _CMD_HELP:
        print_help();
        break;
    case _CMD_CLEAR:
        xprintf("\033[2J\033[H");
        break;
    case _CMD_ECHO:
        if (argc > 1) {
            for (uint8_t i = 1; i < argc; i++) {
                xprintf("%s ", argv[i]);
            }
            xprintf("\n");
        }
        break;
    case _CMD_LUA:
        #ifdef USE_LUA
        if (argc == 2) {
            file_read(argv[1], file_buff, FILE_BUFFER, 0);
            create_lua_task(file_buff);
        }
        #endif
        break;
    case _CMD_TASKS:
        show_monitor();
        break;
    case _CMD_CURR_BLOCK:
        xprintf("%u\n", HaveRunFlashBlockNum());
        break;
    case _CMD_NEOFETCH:
        neofetch();
        break;
    case _CMD_PREF_BLOCK:
        if (argc == 1) {
            xprintf("%u\n", HavePrefFlashBlockNum());
        } else if (argc == 2) {
            long block_num;
            xatoi((char*)argv[1], &block_num);
            FLASH_status status = SetPrefferedBlockNum((uint8_t)block_num);
            if (status != FLASH_OK) {
                xprintf("failed to set preffered block\n");
            }
            system_config.pref_block = (uint64_t)HavePrefFlashBlockNum();
        } else {
            xprintf("pref_block cmd accepts only one argument\n");
        }
        break;
    case _CMD_CHECK_CRC:
        if (argc == 2) {
            long block_num;
            xatoi((char*)argv[1], &block_num);
            uint32_t addr = block_num == 0 ? MAIN_FW_ADDR : RESERVE_FW_ADDR;
            if(CheckBlock((uint32_t *)addr) == 0){
                xprintf("CRC OK\n");
            } else {
                xprintf("CRC INCORRECT\n");
            }
        } else {
            xprintf("check_crc cmd accepts only one argument\n");
        }
        break;
    case _CMD_BEEP:
        if (argc == 3) {
            long freq;
            long duration;
            xatoi((char*)argv[1], &freq);
            xatoi((char*)argv[2], &duration);
            BUZZ_beep(&buzzer, (uint16_t)freq, (uint16_t)duration);
        }
        break;
    case _CMD_CALIB_SENSORS:
        if(argc == 1){
            create_calibration_task();
        }
        break;
    case _CMD_SENS_MEASURE:
        if(argc <= 9){
            args = (Args){.argc = argc, .argv = argv};
            create_sensors_measure_task((void*)(&args));
        }
        break;
    case _CMD_SHOW_CONFIG:
        if(argc <= 2){
            SystemConfig tmp_config;
            if(argc == 2){
                if(strcmp(argv[1], "FLASH") == 0){
                    read_FLASH_system_config(&tmp_config);
                    xprintf("FLASH config:\n");
                    system_config_to_str(&tmp_config, config_json);
                    xprintf(config_json);
                } else if(strcmp(argv[1], "DEFAULT") == 0){
                    system_config_init(&tmp_config);
                    system_config_to_str(&tmp_config, config_json);
                    xprintf("DEFAULT config:\n");
                    xprintf(config_json);
                } else {
                    xprintf("Incorrect argument.\n");
                }
            } else {
                system_config_to_str(&system_config, config_json);
                xprintf("RAM config:\n");
                xprintf(config_json);
            }
        }
        break;
    case _CMD_SET_CONFIG:
        if(argc == 3){
            long val = 0;
            int key_size = 0;
            char path[30] = {0};
            uint16_t config_len = strlen(config_json);
            xsprintf(path, "$.%s", argv[1]);
            if(json_get(config_json, config_len, path, &key_size) < 0){
                xprintf("Key \'%s\' was not found\n", argv[1]);
            } else {
                if(xatoi((char*)argv[2], &val) == 0){
                    xprintf("Incorrect value \'%s\'. Available only integers.\n", argv[2]);
                } else {
                    if(json_set_num(config_json, strlen(config_json), path, val)){
                        xprintf("Failed to set config value\n");
                    } else {
                        parse_system_config(&system_config, config_json, config_len);
                    }
                }
            }
        }
        break;
    case _CMD_SAVE_CONFIG:
        if(argc == 1){
            if(save_system_config_to_FLASH(&system_config) != FLASH_OK)
                xprintf("Saving configuration failed!\n");
            if(save_config_to_SD(&system_config, SYSTEM_CONFIG_PATH, config_json) != CONFIG_OK)
                xprintf("Cant save config to SD\n");
        }
        break;
    case _CMD_CLEAR_CONFIG:
        if(argc == 1){
            system_config_init(&system_config);
            system_config_to_str(&system_config, config_json);
            if(save_system_config_to_FLASH(&system_config) != FLASH_OK){
                xprintf("Saving configuration failed!\n");
            }
            else{
                xprintf("Default config has been written\n");
            }
            if(save_config_to_SD(&system_config, SYSTEM_CONFIG_PATH, config_json) != CONFIG_OK)
                xprintf("Cant save config to SD card\n");
        }
        break;
    case _CMD_SLEEP:
        if(argc == 2){
            long sleep_time = 0;
            xatoi((char*)argv[1], &sleep_time);
            if(sleep_time < 1 || sleep_time > 0xFFFF){
                xprintf("Incorrect sleep time argument\n");
            } else {
                RTC_auto_wakeup_enable((uint16_t)sleep_time);
                stop_cortex();
            }
        }
        break;
    case _CMD_WATCHDOG:
        if(argc == 1){
            while(1){};
        }
        break;
    case _CMD_MOUNT:
        if(argc == 1){
            FATFS fs;
            FRESULT result;
            result = f_mount(&fs, "", 1);
            if (result == FR_OK) {
                xprintf("SD Card mounted\n");
            } else if(SD_Init() == SDR_Success){
                result = f_mount(&fs, "", 1);
                if (result == FR_OK) {
                    xprintf("SD Card mounted\n");
                } else {
                    xprintf("Failed\n");
                }
            } else {
                xprintf("Failed\n");
            }
        }
        break;
    case _CMD_LS:
        if(argc <= 2){
            ls((argc == 2) ? (char *)(argv[1]) : "");
        }
        break;
    case _CMD_CD:
        if(argc == 2){
            memset(curr_dir_name + 26, 0, 102);
            memcpy(curr_dir_name, rl.prompt_str, 26);
            if (f_chdir(argv[1]) != FR_OK) {
                xprintf("FAILED\n");
            } else {
                f_getcwd(curr_dir_name + 26, 91);
                rl.prompt_len = strlen(curr_dir_name) - (27 - _PROMPT_LEN);
                memcpy(curr_dir_name + strlen(curr_dir_name), "\033[37m$\033[0m ", 11);
                rl.prompt_str = curr_dir_name;
            }
        }
        break;
    case _CMD_RENAME:
        if(argc == 3){
            if(f_rename(argv[1], argv[2]) != FR_OK){
                xprintf("Failed\n");
            }
        }
        break;
    case _CMD_RM:
        if(argc == 2){
            if (f_unlink(argv[1]) != FR_OK) {
                xprintf("FAILED\n");
            }
        }
        break;
    case _CMD_CAT:
        if(argc == 2){
            if(file_read(argv[1], file_buff, FILE_BUFFER, 0) != FR_OK){
                xprintf("Failed\n");
            }
            xputc('\n');
        }
        break;
    case _CMD_TAIL:
        if(argc == 3){
            uint32_t offset = 0;
            long str_count = 0;
            xatoi((char*)argv[2], &str_count);
            if(str_count > 0){
                if(tail(argv[1], file_buff, FILE_BUFFER, str_count, &offset) == FR_OK){
                    if(file_read(argv[1], file_buff, FILE_BUFFER, offset) != FR_OK){
                        xprintf("Failed\n");
                    }
                } else {
                    xprintf("Failed\n");
                }
            }
        }
        break;
    case _CMD_HEAD:
        if(argc == 3){
            long str_count = 0;
            xatoi((char *)argv[2], &str_count);
            if(str_count > 0){
                head(argv[1], file_buff, FILE_BUFFER, str_count);
            }
        }
        break;
    case _CMD_DUMP_MEM:
        if(argc == 3){
            long addr = 0;
            long len = 0;
            xatoi((char *)argv[1], &addr);
            xatoi((char *)argv[2], &len);
            if(addr < 0x8000000 || addr > 0x803FFFF){
                xprintf("Address out of range\n");
            } else {
                addr -= MOD(addr, 4);
                dump_memory(addr, (uint32_t *)addr, (size_t)len);
            }
        }
        break;
    case _CMD_DUMP_FILE:
        if(argc == 3){
            long offset = 0;
            xatoi((char *)argv[2], &offset);
            dump_file(argv[1], file_buff, FILE_BUFFER, offset);
        }
        break;
    case _CMD_TOUCH:
        if(argc == 2){
            FIL file;
            if (f_open(&file, argv[1], FA_OPEN_ALWAYS | FA_READ) != FR_OK) {
                xprintf("FAILED\n");
            } else {
                f_close(&file);
            }
        }
        break;
    case _CMD_MKDIR:
        if(argc == 2){
            if(f_mkdir(argv[1]) != FR_OK){
                xprintf("FAILED\n");
            }
        }
        break;
    case _CMD_RESET:
        __NVIC_SystemReset();
        break;
    case _CMD_TIME:
        if (argc == 1) {
            RTC_string_datetime(rtc_buffer);
            xprintf("%s\n", rtc_buffer);
        } else if (argc == 7) {
            long years;
            long months;
            long date;
            long hours;
            long minutes;
            long seconds;
            xatoi((char *)argv[1], &years);
            xatoi((char *)argv[2], &months);
            xatoi((char *)argv[3], &date);
            xatoi((char *)argv[4], &hours);
            xatoi((char *)argv[5], &minutes);
            xatoi((char *)argv[6], &seconds);
            RTC_struct_brief new_time = {.years = years > 2000 ? years - 2000 : years,
                .months = months,
                .date = date,
                .hours = hours,
                .minutes = minutes,
                .seconds = seconds,
                .sub_seconds = 0};
                RTC_data_update(&new_time);
                RTC_string_datetime(rtc_buffer);
                xprintf("%s\n", rtc_buffer);
            } else {
                xprintf("incorrect arguments for \'time\' cmd\n");
            }
            break;
    case _CMD_RADIO_CONF:
        if (argc == 1) {
            xprintf("Reg num: Value\n");
            for (uint8_t i = 0; i < 127; i++) {
                uint8_t reg = LoRa_readRegister(&sx127x, i);
                xprintf("0x%0X: 0x%0X\n", i, reg);
            }
            xprintf("LoRa settings:\nFrequency: %d\nSF: %d\nBW: %d\nCR: %d\nOutPow: %d\n",
                    sx127x.freq_mhz, sx127x.spredingFactor, sx127x.bandWidth, sx127x.codingRate, sx127x.power);
        } else if (argc == 7) {
            long bandWidth = 0;
            long freq_mhz = 0;
            long spredingFactor = 0;
            long codingRate = 0;
            long power = 0;
            xatoi((char *)argv[1], &bandWidth);
            xatoi((char *)argv[2], &freq_mhz);
            xatoi((char *)argv[3], &spredingFactor);
            xatoi((char *)argv[4], &codingRate);
            xatoi((char *)argv[5], &power);
            sx127x.bandWidth = (uint8_t)bandWidth;
            sx127x.freq_mhz = (uint32_t)freq_mhz;
            sx127x.spredingFactor = (uint8_t)spredingFactor;
            sx127x.codingRate = (uint8_t)codingRate;
            sx127x.power = (uint8_t)power;
        } else {
            xprintf("you should set only frequency and duration\n");
        }
        break;
    case _CMD_SEND_RADIO:
        if (argc == 2) {
            LoRa_transmit(&sx127x, (uint8_t *)argv[1], strlen(argv[1]));
        }
        break;
    case _CMD_XMODEM:
        if (argc == 1) {
            xmodem.save = xmodem_flash_write;
            xmodem.on_first_packet = FLASH_erase_neighbor;
            uint32_t write_addr = HaveRunFlashBlockNum() != 0 ? MAIN_FW_ADDR : RESERVE_FW_ADDR;
            xmodem_receive(&xmodem, write_addr);
        } else if (argc == 2){
            FIL file;
            xmodem.save = xmodem_sd_write;
            xmodem.on_first_packet = xmodem_sd_on_first_packet;
            FRESULT res = f_open(&file, argv[1], FA_CREATE_NEW | FA_WRITE);
            if(res != FR_OK){
                f_close(&file);
                xprintf("Failed to create file\n");
                return 1;
            }
            xmodem_receive(&xmodem, 0);
            xprintf("Written %d bytes to file\n", f_size(&file));
            f_close(&file);
        } else {
            xprintf("Incorrect amount of arguments\n");
        }
        break;
    case _CMD_ERASE_FIRMWARE:
        if(FLASH_erase_firmware(HaveRunFlashBlockNum()) != FLASH_OK)
            xprintf("Failed\n");
        break;
    case _CMD_UPLOAD_SD_FW:
        if (argc == 2) {
            uint32_t write_addr = HaveRunFlashBlockNum() != 0 ? MAIN_FW_ADDR : RESERVE_FW_ADDR;
            xprintf("Writing to address %08lX\n", write_addr);
            if(copy_to_flash(argv[1], file_buff, FILE_BUFFER, write_addr) != FR_OK){
                xprintf("Failed\n");
            }
        }
        break;
    case _CMD_TOGGLE_GSM:
        GSM_TogglePower(&sim7000g);
        break;
    case _CMD_TCP_INIT:
        GSM_CheckSIM(&sim7000g);
        sim7000g.delay_ms(500);
        GSM_CheckGSM(&sim7000g);
        sim7000g.delay_ms(200);
        if(GSM_InitGPRS(&sim7000g) == 0){
            xSemaphoreTake(xSemaphore, 1000);
            xprintf("Successfully inited GPRS\n");
            xSemaphoreGive(xSemaphore);
        }
        else {
            xSemaphoreTake(xSemaphore, 1000);
            xprintf("Failed GPRS initialization\n");
            xSemaphoreGive(xSemaphore);
        }
        break;
    case _CMD_TCP_OPEN:
        if(argc == 1){
            char port[6] = {0};
            xsprintf(port, "%d", system_config.port);
            GSM_OpenConnection(&sim7000g, system_config.ip, port);
        }
        else if (argc == 3) {
            GSM_OpenConnection(&sim7000g, argv[1], argv[2]);
        }
        break;
    case _CMD_TCP_CLOSE:
        GSM_CloseConnections(&sim7000g);
        break;
    case _CMD_RUN_ACTION:
        create_action_task();
        break;
    case _CMD_TCP_SEND:
        if (argc == 2) {
            GSM_SendTCP(&sim7000g, argv[1], strlen(argv[1]));
        } else if (argc == 3){
            long size = 0;
            xatoi((char*)argv[2], &size);
            GSM_SendFile(&sim7000g, (char*)argv[1], size);
        }
        break;
    default:
        if (strstr(argv[0], "at") != 0 || strstr(argv[0], "AT") != 0) {
            if (argc == 1) {
                GSM_SendCMD(&sim7000g, (char*)argv[0]);
            }
        } else {
            xprintf("Unknown command. Type \"help\" for show list of commands\n");
        }
        break;
    }
    return 0;
}


#ifdef _USE_COMPLETE
//*****************************************************************************
// completion callback for microrl library
char **complet(int argc, const char *const *argv) {
    int j = 0;

    compl_word[0] = NULL;

    // if there is token in cmdline
    if (argc == 1) {
        // get last entered token
        char *bit = (char *)argv[argc - 1];
        // iterate through our available token and match it
        for (int i = 0; i < _NUM_OF_CMD; i++) {
            // if token is matched (text is part of our token starting from 0 char)
            if (strstr(keyword[i], bit) == keyword[i]) {
                // add it to completion set
                compl_word[j++] = keyword[i];
            }
        }
        // } else if ((argc > 1) && ((strcmp(argv[0], _CMD_SET) == 0) ||
        //                           (strcmp(argv[0], _CMD_CLR) == 0))) {  // if command needs subcommands
        //     // iterate through subcommand
        //     for (int i = 0; i < _NUM_OF_SETCLEAR_SCMD; i++) {
        //         if (strstr(set_clear_key[i], argv[argc - 1]) == set_clear_key[i]) {
        //             compl_world[j++] = set_clear_key[i];
        //         }
        //     }
    } else {  // if there is no token in cmdline, just print all available token
        for (; j < _NUM_OF_CMD; j++) {
            compl_word[j] = keyword[j];
        }
    }

    // note! last ptr in array always must be NULL!!!
    compl_word[j] = NULL;
    // return set of variants
    return compl_word;
}
#endif

//*****************************************************************************
void sigint(void) {
    char *tasks[] = {
        "TEMP_CALIB",
        "LUA",
        "SENS_MEASURE",
        "TEMP_CALIB",
        // "ACTION_TASK"
    };
    for(uint8_t i = 0; i < 4; i++){
        TaskHandle_t task_handle = xTaskGetHandle(tasks[i]);
        if(task_handle != NULL){
            vTaskDelete(task_handle);
            xprintf("\n%d cancelled!\n\r", tasks[i]);
        }
    }
    xprintf("\n^C catched!\n\r");
    xprintf(_PROMPT_DEFAULT);
    microrl_clear_input(rl);
}