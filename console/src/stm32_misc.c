#include "stm32_misc.h"

#include <string.h>
#include <xprintf.h>
#include "main.h"
#include "../inc/config.h"
#include "buzzer.h"
#include "ff.h"
#include "flash.h"
#include "monitor_task.h"
#include "rtc.h"
#include "sx127x.h"
#include "sx127x_misc.h"
#include "system_select.h"
#include "xmodem.h"
#include "global_variables.h"
#include "ds18b20_bus.h"


#define _STM_DEMO_VER "1.0"

// definition commands word
#define _CMD_HELP "help"                      // 1
#define _CMD_CLEAR "clear"                    // 2
#define _CMD_ECHO "echo"                      // 3
#define _CMD_BEEP "beep"                      // 4
#define _CMD_RESET "reset"                    // 5
#define _CMD_TASKS "tasks"                    // 6
#define _CMD_XMODEM "xmodem"                  // 7
#define _CMD_PREF_BLOCK "pref_block"          // 8
#define _CMD_CURR_BLOCK "curr_block"          // 9
#define _CMD_ERASE_FIRMWARE "erase_firmware"  // 10
#define _CMD_CHECK_CRC "check_crc"            // 11
#define _CMD_SEND_RADIO "send_radio"          // 12
#define _CMD_RADIO_CONF "radio_conf"          // 13
#define _CMD_MAKE_MEASURE "make_measure"      // 14
#define _CMD_REC_SENSORS "rec_sensors"        // 15
#define _CMD_SLEEP "sleep"                    // 16
#define _CMD_TIME "time"                      // 17
#define _CMD_LS "ls"                          // 18
#define _CMD_CD "cd"                          // 19
#define _CMD_RM "rm"                          // 19
#define _CMD_RENAME "rename"                  // 19
#define _CMD_TOUCH "touch"                    // 21
#define _CMD_MKDIR "mkdir"                    // 23
#define _CMD_CAT "cat"                        // 24
#define _CMD_TAIL "tail"                      // 25
#define _CMD_UPLOAD_SD_FW "upload_sd_fw"      // 26

#define _NUM_OF_CMD 26
#define _NUM_OF_SETCLEAR_SCMD 2

#define AQUA_COLOR(STRING_VAL) "\033[36m" STRING_VAL "\033[0m"

// available  commands
char *keyworld[] = {_CMD_HELP,              // 1
                    _CMD_CLEAR,             // 2
                    _CMD_ECHO,              // 3
                    _CMD_BEEP,              // 4
                    _CMD_RESET,             // 5
                    _CMD_TASKS,             // 6
                    _CMD_XMODEM,            // 7
                    _CMD_PREF_BLOCK,        // 8
                    _CMD_CURR_BLOCK,        // 9
                    _CMD_TIME,              // 10
                    _CMD_ERASE_FIRMWARE,    // 11
                    _CMD_CHECK_CRC,         // 12
                    _CMD_SEND_RADIO,        // 13
                    _CMD_MAKE_MEASURE,      // 14
                    _CMD_REC_SENSORS,       // 15
                    _CMD_SLEEP,             // 16
                    _CMD_RADIO_CONF,        // 17
                    _CMD_LS,                // 18
                    _CMD_CD,                // 19
                    _CMD_RM,                // 20
                    _CMD_RENAME,            // 21
                    _CMD_TOUCH,             // 22
                    _CMD_MKDIR,             // 23
                    _CMD_CAT,               // 24
                    _CMD_TAIL,              // 25
                    _CMD_UPLOAD_SD_FW};     // 26
// 'set/clear' command argements

// array for comletion
char *compl_world[_NUM_OF_CMD + 1];

void put_char(unsigned char ch);

//*****************************************************************************
void init(void);

void print(const char *str) { xprintf(str); }

//*****************************************************************************
void print_help(void) {
    print("Use TAB key for completion\n\rCommand:\n\r");
    print("  clear                          - clear screen\n\r");
    print("  echo [data]                    - print arg to screen\n\r");
    print("  ls [path]                      - show directory content'\n\r");
    print("  cd [path]                      - change directory\n\r");
    print("  rm  [dir_or_file_name]         - remove file or directory\n\r");
    print("  touch [file_name]              - create file\n\r");
    print("  rename [old_name] [new_name]   - rename file\n\r");
    print("  mkdir [dir_name]               - create directory\n\r");
    print("  cat [file_name]                - read/write file\n\r");
    print("  tail [file_name]               - read/write file\n\r");
    print("  reset                          - reset MCU\n\r");
    print("  tasks                          - show tasks\n\r");
    print("  xmodem                         - start flashing procedure over XModem protocol\n\r");
    print("  pref_block [block_num]         - get/set firmware preffered block\n\r");
    print("  curr_block                     - get current firmware block\n\r");
    print("  erase_firmware                 - erase firmware \n\r");
    print("  check_crc [block_num]          - check firmware CRC \n\r");
    print("  upload_sd_fw [file_name]       - write firmware to FLASH from SD card\n\r");
    print("  make_measure                   - measure temperature and moisture\n\r");
    print("  rec_sensors                    - start temperature sensors recognition routine\n\r");
    print("  send_radio [*data]             - send LoRa package \n\r");
    print("  radio_conf [sf] [bw] [cr] [crc_en] [ldro] [tx_pow] - show/set LoRa configuration\n\r");
    print("  time [year] [month] [day] [hours] [mins] [seconds] - get/set MCU RTC\n\r");
    print("  beep [freq] [duration]         - make beep signal certain frequency and duration\n\r");
    print("  sleep [sleep_sec]              - go to sleep mode\n\r");
}

FRESULT res;
char buff[513] = {0};
// DS18B20_BUS sensors_bus;

void ls(const char *path) {
    DIR dir;
    char string[256];
    res = f_opendir(&dir, path);

    if (res == FR_OK) {
        print("Mode            Last write       Size   Name\n\r");
        while (1) {
            FILINFO fno;

            res = f_readdir(&dir, &fno);
            if(res == FR_DISK_ERR){
                print("DISK ERROR\n\r");
                break;
            }

            if ((res != FR_OK) || (fno.fname[0] == 0))
                break;
            char file_label[128] = " ";
            // xsprintf(string, "%c%c%c%c%c %u/%02u/%02u %02u:%02u %10d %s/%s\n\r",
            if((fno.fattrib & AM_DIR)){
                memmove(file_label, "\033[36m", 5);
                memmove(file_label + 5,  fno.fname, strlen(fno.fname));
                memmove(file_label + 5 + strlen(fno.fname), "\033[0m", 4);
            }
            xsprintf(string, "%c%c%c%c%c     %u/%02u/%02u %02u:%02u %10d   %s\n\r",
                     ((fno.fattrib & AM_DIR) ? 'D' : '-'),
                     ((fno.fattrib & AM_RDO) ? 'R' : '-'),
                     ((fno.fattrib & AM_SYS) ? 'S' : '-'),
                     ((fno.fattrib & AM_HID) ? 'H' : '-'),
                     ((fno.fattrib & AM_ARC) ? 'A' : '-'),
                     (fno.fdate >> 9) + 1980, (fno.fdate >> 5) & 15,
                     fno.fdate & 31, (fno.ftime >> 11), (fno.ftime >> 5) & 63,
                    //  (int)fno.fsize, path, fno.fname);
                     (int)fno.fsize, (fno.fattrib & AM_DIR) ? file_label : fno.fname);

            print(string);
        }
    } else {
        print("Can\'t open dir\n\r");
    }
}

uint8_t forward_uart(char *buff){
    print(buff);
    return 0;
}

FRESULT file_read(const char *path, char *buff, uint16_t buff_size,
                  uint32_t offset, uint8_t (*forward)(char *)) {
    FIL file;
    UINT read_count = 0;
    res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
    if(res != FR_OK){
        f_close(&file);
        print("Failed\n\r");
        return res;
    }
    while(f_size(&file) > offset){
        memset(buff, 0, buff_size);
        res = f_lseek(&file, offset);
        if(res != FR_OK) break;
        res = f_read(&file, (void *)(buff), buff_size, &read_count);
        offset += read_count;
        if(res != FR_OK) break;
        if(!forward(buff)){
            res = FR_INT_ERR;
            break;
        }
    }
    f_close(&file);
    return res;
}

char curr_dir_name[128] = "";
char rtc_buffer[20] = {0};
//*****************************************************************************
// execute callback for microrl library
// do what you want here, but don't write to argv!!! read only!!
int execute(int argc, const char *const *argv) {
    int i = 0;
    // just iterate through argv word and compare it with your commands
    while (i < argc) {
        if (strcmp(argv[i], _CMD_HELP) == 0) {
            print("microrl v" MICRORL_LIB_VER " library STM32 DEMO v" _STM_DEMO_VER "\n\r");
            print_help();  // print help
            break;
        } else if (strcmp(argv[i], _CMD_CLEAR) == 0) {
            print("\033[2J");  // ESC seq for clear entire screen
            print("\033[H");   // ESC seq for move cursor at left-top corner
        } else if (strcmp(argv[i], _CMD_ECHO) == 0) {
            if (argc > 1) {
                for (i = 1; i < argc; i++) {
                    print(argv[i]);
                    print(" ");
                }
                print("\n\r");
            }
        } else if (strcmp(argv[i], _CMD_TASKS) == 0) {
            if (argc == 1) {
                show_monitor();
                break;
            } else {
                print("reset cmd does not accept any arguments\n\r");
            }
        } else if (strcmp(argv[i], _CMD_CURR_BLOCK) == 0) {
            if (argc == 1) {
                xprintf("%u\n\r", HaveRunFlashBlockNum());
                break;
            } else {
                print("get_curr_block cmd does not accept any arguments\n\r");
            }
        } else if (strcmp(argv[i], _CMD_PREF_BLOCK) == 0) {
            if (argc == 1) {
                xprintf("%u\n\r", HavePrefFlashBlockNum());
                break;
            } else if (argc == 2) {
                long block_num;
                xatoi((char **)(&argv[1]), &block_num);
                uint8_t status = SetPrefferedBlockNum((uint8_t)block_num);
                if (status) {
                    print("failed to set preffered block\n\r");
                }
                break;
            } else {
                print("set_pref_block cmd accepts only one argument\n\r");
            }
        } else if (strcmp(argv[i], _CMD_CHECK_CRC) == 0) {
            if (argc == 2) {
                long block_num;
                xatoi((char **)(&argv[1]), &block_num);
                uint32_t addr = block_num == 0 ? 0x8000000 : 0x8010000;
                xprintf("%d\n\r", CheckBlock((uint32_t *)addr));
                break;
            } else {
                print("check_crc cmd accepts only one argument\n\r");
            }
        } else if (strcmp(argv[i], _CMD_BEEP) == 0) {
            if (++i < argc) {
                if (argc == 3) {
                    long freq;
                    long duration;
                    xatoi((char **)(&argv[1]), &freq);
                    xatoi((char **)(&argv[2]), &duration);
                    BUZZ_beep(&buzzer, (uint16_t)freq, (uint16_t)duration);
                } else {
                    print("you should set only frequency and duration\n\r");
                }
                break;
            }
        } else if (strcmp(argv[i], _CMD_MAKE_MEASURE) == 0) {
            if(argc == 1){
                gpio_state(EN_PERIPH, LOW);
                uint8_t dev_num = OneWire_SearchDevices(sensors_bus.ow);
                xprintf("devices: %d\n\r", dev_num);
                TemperatureSensorsMeasure(&sensors_bus, 0);
                gpio_state(EN_PERIPH, HIGH);
                for(uint8_t i = 0; i < 12; i++){
                    xprintf("%.2f\t", sensors_bus.sensors[i].temperature);
                }
                print("\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_REC_SENSORS) == 0) {
            if(argc == 1){
                gpio_state(EN_PERIPH, LOW);
                float initial_temps[12] = {0};
                uint8_t sorted_nums[12] = {0};
                RecognitionRoutine(&sensors_bus, initial_temps, sorted_nums);
                gpio_state(EN_PERIPH, HIGH);
            }
            break;
        } else if (strcmp(argv[i], _CMD_LS) == 0) {
            if(argc <= 2){
                ls((argc == 2) ? (char *)(argv[1]) : "");
            }
            break;
        } else if (strcmp(argv[i], _CMD_CD) == 0) {
            if(argc == 2){

                memset(curr_dir_name + 26, 0, 102);
                memcpy(curr_dir_name, rl.prompt_str, 26);
                if (f_chdir(argv[1]) != FR_OK) {
                    print("FAILED\n\r");
                } else {
                    f_getcwd(curr_dir_name + 26, 91);
                    rl.prompt_len = strlen(curr_dir_name) - (27 - _PROMPT_LEN);
                    memcpy(curr_dir_name + strlen(curr_dir_name), "\033[37m$\033[0m ", 11);
                    rl.prompt_str = curr_dir_name;
                }
            } else {
                print("You need to set only \"file_name\" argument\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_RENAME) == 0) {
            if(argc == 3){
                if(f_rename(argv[1], argv[2]) != FR_OK){
                    print("Failed\n\r");
                }
            } else {
                print("Incorrect arguments for \'rename\'\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_RM) == 0) {
            if(argc == 2){
                if (f_unlink(argv[1]) != FR_OK) {
                    print("FAILED\n\r");
                }
            } else {
                print("You need to set only \"file_name\" argument\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_CAT) == 0) {
            if(argc > 1 && argc < 3){
                if(argc == 2){
                    if(file_read(argv[1], buff, 512, 0, forward_uart) != FR_OK){
                        print("Failed\n\r");
                    }

                }
            }
            break;
        } else if (strcmp(argv[i], _CMD_TAIL) == 0) {
            if(argc > 1 && argc < 3){

            }
            break;
        } else if (strcmp(argv[i], _CMD_TOUCH) == 0) {
            FIL file;
            if(argc == 2){
                if (f_open(&file, argv[1], FA_OPEN_ALWAYS | FA_READ) != FR_OK) {
                    print("FAILED\n\r");
                } else {
                    f_close(&file);
                }
            } else {
                print("You need to set only \"file_name\" argument\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_MKDIR) == 0) {
            if(argc == 2){
                if(f_mkdir(argv[1]) != FR_OK){
                    print("FAILED\n\r");
                }
            } else {
                print("You need to set only \"dir_name\" argument\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_RESET) == 0) {
            if (argc == 1) {
                __NVIC_SystemReset();
            } else {
                print("reset cmd does not accept any arguments\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_TIME) == 0) {
            if (argc == 1) {
                RTC_string_datetime(rtc_buffer);
                print(rtc_buffer);
                print("\n\r");
            } else if (argc == 7) {
                long years;
                long months;
                long date;
                long hours;
                long minutes;
                long seconds;
                xatoi((char **)(&argv[1]), &years);
                xatoi((char **)(&argv[2]), &months);
                xatoi((char **)(&argv[3]), &date);
                xatoi((char **)(&argv[4]), &hours);
                xatoi((char **)(&argv[5]), &minutes);
                xatoi((char **)(&argv[7]), &seconds);
                RTC_struct_brief new_time = {.years = years > 2000 ? years - 2000 : years,
                                             .months = months,
                                             .date = date,
                                             .hours = hours,
                                             .minutes = minutes,
                                             .seconds = seconds,
                                             .sub_seconds = 0};
                RTC_data_update(&new_time);
                RTC_string_datetime(rtc_buffer);
                print(rtc_buffer);
                print("\n\r");
            } else {
                print("incorrect arguments for \'time\' cmd\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_RADIO_CONF) == 0) {
            if (argc == 1) {
                print("Reg num: Value\n\r");
                for (uint8_t i = 0; i < 127; i++) {
                    uint8_t reg = LoRa_readRegister(&sx127x, i);
                    xprintf("0x%0X: 0x%0X\n\r", i, reg);
                }
                xprintf("LoRa settings:\n\rFrequency: %d\n\rSF: %d\n\rBW: %d\n\rCR: %d\n\rOutPow: %d\n\r",
                        sx127x.freq_mhz, sx127x.spredingFactor, sx127x.bandWidth, sx127x.codingRate, sx127x.power);
            } else if (argc == 7) {
                long bandWidth = 0;
                long freq_mhz = 0;
                long spredingFactor = 0;
                long codingRate = 0;
                long power = 0;
                xatoi((char **)(&argv[1]), &bandWidth);
                xatoi((char **)(&argv[2]), &freq_mhz);
                xatoi((char **)(&argv[3]), &spredingFactor);
                xatoi((char **)(&argv[4]), &codingRate);
                xatoi((char **)(&argv[5]), &power);
                sx127x.bandWidth = (uint8_t)bandWidth;
                sx127x.freq_mhz = (uint32_t)freq_mhz;
                sx127x.spredingFactor = (uint8_t)spredingFactor;
                sx127x.codingRate = (uint8_t)codingRate;
                sx127x.power = (uint8_t)power;
            } else {
                print("you should set only frequency and duration\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_SEND_RADIO) == 0) {
            if (argc == 2) {
                LoRa_transmit(&sx127x, (uint8_t *)argv[1], strlen(argv[1]));
            } else {
                print("you should set only message string for transaction\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_XMODEM) == 0) {
            if (argc == 1) {
                uint32_t write_addr = HaveRunFlashBlockNum() != 0 ? MAIN_FW_ADDR : RESERVE_FW_ADDR;
                xmodem_receive(write_addr);
            } else {
                print("xmodem cmd does not accept any arguments\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_ERASE_FIRMWARE) == 0) {
            if (argc == 1) {
                uint8_t page_offset = 0;
                uint8_t page_amount = 32;
                FLASH_status status = FLASH_OK;
                if (HaveRunFlashBlockNum() == 1) {
                    for (uint8_t i = 0; i < page_amount && status == FLASH_OK; i++) {
                        status = FLASH_erase_page(page_offset + i);
                    }
                } else {
                    page_offset = 32;
                    for (uint8_t i = 0; i < page_amount && status == FLASH_OK; i++) {
                        status = FLASH_erase_page(page_offset + i);
                    }
                }
            } else {
                print("reset cmd does not accept any arguments\n\r");
            }
            break;
        } else if (strcmp(argv[i], _CMD_UPLOAD_SD_FW) == 0) {
            if (argc == 2) {

            }
        } else {
            print("Incorerct arguments");
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

    compl_world[0] = NULL;

    // if there is token in cmdline
    if (argc == 1) {
        // get last entered token
        char *bit = (char *)argv[argc - 1];
        // iterate through our available token and match it
        for (int i = 0; i < _NUM_OF_CMD; i++) {
            // if token is matched (text is part of our token starting from 0 char)
            if (strstr(keyworld[i], bit) == keyworld[i]) {
                // add it to completion set
                compl_world[j++] = keyworld[i];
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
            compl_world[j] = keyworld[j];
        }
    }

    // note! last ptr in array always must be NULL!!!
    compl_world[j] = NULL;
    // return set of variants
    return compl_world;
}
#endif

//*****************************************************************************
void sigint(void) { print("^C catched!\n\r"); }