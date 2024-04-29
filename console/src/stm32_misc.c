#include "stm32_misc.h"
#include <string.h>
#include <xprintf.h>
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

#define MOD(x, y) (x & (y - 1))  // y must be power of 2!


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
#define _CMD_TEMPS_MEASURE "temps_measure"      // 14
#define _CMD_ADC_MEASURE "adc_measure"      // 14
#define _CMD_CALIB_SENSORS "calib_sensors"    // 15
#define _CMD_ID "id"                          // 16
#define _CMD_SLEEP "sleep"                    // 17
#define _CMD_TIME "time"                      // 18
#define _CMD_ACTION_MODE "action_mode"        // 19
#define _CMD_DUMP_MEM "dump_mem"              // 20
#define _CMD_MOUNT "mount"                    // 21
#define _CMD_NEOFETCH "neofetch"              // 22
#define _CMD_LS "ls"                          // 23
#define _CMD_CD "cd"                          // 24
#define _CMD_RM "rm"                          // 25
#define _CMD_RENAME "rename"                  // 26
#define _CMD_TOUCH "touch"                    // 27
#define _CMD_MKDIR "mkdir"                    // 28
#define _CMD_CAT "cat"                        // 29
#define _CMD_TAIL "tail"                      // 30
#define _CMD_HEAD "head"                      // 31
#define _CMD_DF "df"                          // 32
#define _CMD_SAVE_CONFIG "save_config"        // 33
#define _CMD_UPLOAD_SD_FW "upload_sd_fw"      // 34

#define _NUM_OF_CMD 35

void uart_print(int data){
    UART_tx(USART_PRINT, (uint8_t)(data));
}
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
                    _CMD_TEMPS_MEASURE,      // 14
                    _CMD_ADC_MEASURE,      // 14
                    _CMD_CALIB_SENSORS,     // 15
                    _CMD_ID,                // 16
                    _CMD_SLEEP,             // 18
                    _CMD_RADIO_CONF,        // 19
                    _CMD_DUMP_MEM,          // 20
                    _CMD_NEOFETCH,          // 21
                    _CMD_LS,                // 21
                    _CMD_CD,                // 22
                    _CMD_RM,                // 23
                    _CMD_DF,                // 24
                    _CMD_RENAME,            // 25
                    _CMD_TOUCH,             // 26
                    _CMD_MKDIR,             // 27
                    _CMD_CAT,               // 28
                    _CMD_TAIL,              // 29
                    _CMD_HEAD,              // 30
                    _CMD_MOUNT,             // 31
                    _CMD_ACTION_MODE,       // 32
                    _CMD_SAVE_CONFIG,       // 33
                    _CMD_UPLOAD_SD_FW};     // 34

// array for comletion
char *compl_world[_NUM_OF_CMD + 1];



//*****************************************************************************
/*
need to implements:
    df, mount, head, id
*/
void print_help(void) {
    xprintf("microrl v" MICRORL_LIB_VER " library STM32 DEMO v" _STM_DEMO_VER "\n"\
            "Use TAB key for completion\nCommand:\n"\
      /*2*/   "  clear                          - clear screen\n"\
      /*3*/   "  echo [data]                    - print arg to screen\n"\
      /*4*/   "  ls [path]                      - show directory content\n"\
      /*5*/   "  neofetch                       - show system info'\n"\
      /*6*/   "  cd [path]                      - change directory\n"\
      /*7*/   "  rm  [dir_or_file_name]         - remove file or directory\n"\
      /*8*/   "  touch [file_name]              - create file\n"\
      /*9*/   "  rename [old_name] [new_name]   - rename/move file\n"\
      /*10*/   "  mkdir [dir_name]               - create directory\n"\
      /*11*/  "  cat [file_name]                - read/write file\n"\
      /*12*/  "  tail [file_name]               - read last string of file\n"\
      /*13*/  "  head [file_name]               - read first strings of file\n"\
      /*14*/  "  reset                          - reset MCU\n"\
      /*15*/  "  df                             - show disk status\n"\
      /*16*/  "  mount                          - mount SD card\n"\
      /*17*/  "  dump_mem [addr] [length]       - dump memory slice\n"\
      /*18*/  "  tasks                          - show tasks\n"\
      /*19*/  "  xmodem                         - start flashing procedure over XModem protocol\n"\
      /*20*/  "  pref_block [block_num]         - get/set firmware preffered block\n"\
      /*21*/  "  curr_block                     - get current firmware block\n"\
      /*22*/  "  erase_firmware                 - erase firmware \n"\
      /*23*/  "  save_config [autosave]         - save/autosave system configuration to FLASH\n"\
      /*24*/  "  check_crc [block_num]          - check firmware CRC \n"\
      /*25*/  "  upload_sd_fw [file_name]       - write firmware to FLASH from SD card\n"\
      /*26*/  "  temps_measure                  - measure temperature sensors\n"\
      /*26*/  "  adc_measure                    - measure ADC value of moisture sensors\n"\
      /*27*/  "  calib_sensors  [-recalib]      - start temperature sensors calibration routine\n"\
      /*28*/  "  id [id]                        - set/get module identificator\n"\
      /*29*/  "  action_mode  [mode]            - set/get action mode\n"\
      /*30*/  "  send_radio [*data]             - send LoRa package \n"\
      /*31*/  "  radio_conf [sf] [bw] [cr] [crc_en] [ldro] [tx_pow] - show/set LoRa configuration\n"\
      /*32*/  "  time [year] [month] [day] [hours] [mins] [seconds] - get/set MCU RTC\n"\
      /*33*/  "  beep [freq] [duration]         - make beep signal certain frequency and duration\n"\
      /*34*/  "  sleep [sleep_sec]              - go to sleep mode\n");
}

FRESULT res;
FIL file;
char file_buff[512] = {0};

// DS18B20_BUS sensors_bus;

void neofetch(){
    xprintf(
"\033[33m              .,-:;//;:=,                 \033[32m root\033[0m@\033[32mstm32\n"\
"\033[33m          . :H@@@MM@M#H/.,+%%;,           \033[0m  -----------\n"\
"\033[33m       ,/X+ +M@@M@MM%%=,-%%HMMM@X/,       \033[36m   OS\033[0m: FreeRTOS \n"\
"\033[33m     -+@MM; $M@@MH+-,;XMMMM@MMMM@+-       \033[36m Host\033[0m: STM32\n"\
"\033[33m    ;@M@@M- XM@X;. -+XXXXXHHH@M@M#@/.     \033[36m Kernel\033[0m: V202212.01\n"\
"\033[33m  ,%%MM@@MH ,@%%=             .---=-=:=,. \033[36m   Uptime\033[0m: \n"\
"\033[33m  =@#@@@MX.,                -%%HX$$%%%%:; \033[36m    Shell\033[0m: microrl v." MICRORL_LIB_VER "\n"\
"\033[33m =-./@M@M$                   .;@MMMM@MM:  \033[36m MCU\033[0m: STM32L431RCT6\n"\
"\033[33m X@/ -$MM/                    . +MM@@@M$  \033[36m Memory\033[0m: 71KB/128KB\n"\
"\033[33m,@M@H: :@:                    . =X#@@@@-  \033[36m Battery\033[0m: 100%%\n"\
"\033[33m,@@@MMX, .                    /H- ;@M@M=   \033[0m\n"\
"\033[33m.H@@@@M@+,                    %%MM+..%%#$.   \033[40m   \033[41m   \033[42m   \033[43m   \033[44m   \033[45m   \033[46m   \033[47m   \033[0m\n"\
"\033[33m /MMMM@MMH/.                  XM@MH; =;    \033[100m   \033[101m   \033[102m   \033[103m   \033[104m   \033[105m   \033[106m   \033[107m   \033[0m\n"\
"\033[33m  /%%+%%$XHH@$=              , .H@@@@MX,\033[0m\n"\
"\033[33m   .=--------.           -%%H.,@@@@@MX,\033[0m\n"\
"\033[33m   .%%MM@@@HHHXX$$$%%+- .:$MMX =M@@MM%%.\033[0m\n"\
"\033[33m     =XMMM@MM@MM#H;,-+HMM@M+ /MMMX=\033[0m\n"\
"\033[33m       =%%@M@M#@$-.=$@MM@@@M; %%M%%=\033[0m\n"\
"\033[33m         ,:+$+-,/H#MMMMMMM@= =,\033[0m\n"\
"\033[33m              =++%%%%+/:-.\033[0m\n"\
);
}

void ls(const char *path) {
    DIR dir;
    res = f_opendir(&dir, path);

    if (res == FR_OK) {
        xprintf("Mode            Last write       Size   Name\n");
        while (1) {
            FILINFO fno;

            res = f_readdir(&dir, &fno);
            if(res == FR_DISK_ERR){
                xprintf("DISK ERROR\n");
                break;
            }

            if ((res != FR_OK) || (fno.fname[0] == 0))
                break;
            char file_label[128] = " ";
            // xsprintf(string, "%c%c%c%c%c %u/%02u/%02u %02u:%02u %10d %s/%s\n",
            if((fno.fattrib & AM_DIR)){
                memmove(file_label, "\033[36m", 5);
                memmove(file_label + 5,  fno.fname, strlen(fno.fname));
                memmove(file_label + 5 + strlen(fno.fname), "\033[0m", 4);
            }
            xprintf("%c%c%c%c%c     %u/%02u/%02u %02u:%02u %10d   %s\n",
                     ((fno.fattrib & AM_DIR) ? 'D' : '-'),
                     ((fno.fattrib & AM_RDO) ? 'R' : '-'),
                     ((fno.fattrib & AM_SYS) ? 'S' : '-'),
                     ((fno.fattrib & AM_HID) ? 'H' : '-'),
                     ((fno.fattrib & AM_ARC) ? 'A' : '-'),
                     (fno.fdate >> 9) + 1980, (fno.fdate >> 5) & 15,
                     fno.fdate & 31, (fno.ftime >> 11), (fno.ftime >> 5) & 63,
                    //  (int)fno.fsize, path, fno.fname);
                     (int)fno.fsize, (fno.fattrib & AM_DIR) ? file_label : fno.fname);
        }
    } else {
        xprintf("Can\'t open dir\n");
    }
}

void dump_memory(unsigned long addr, uint32_t *ptr, size_t len){
    xprintf("%10s | %8s | %8s | %8s | %8s | %16s\n", "Address", "0", "4", "8", "C", "ASCII");
    for(uint32_t i = 0; i < len; i += 4){
        // xprintf("0x%08lX | %08lX | %08lX | %08lX | %08lX | ",
        //         addr + (i << 2), *(ptr + i), *(ptr + i + 1), *(ptr + i + 2), *(ptr + i + 3));
        xprintf("0x%08lX | ", addr + (i << 2));
        for(uint8_t c = 0; c < 4; c++){
            unsigned long val = *(ptr + i + c);
            if(val == 0){
                xprintf("\033[36m");
            } else if (val == 0xFFFFFFFF){
                xprintf("\033[33m");
            }
            xprintf("%08lX", val);
            if(val == 0 || val == 0xFFFFFFFF){
                xprintf("\033[0m");
            }
            xprintf(" | ");
        }
        for (uint8_t j = 0; j < 16; j++) {		/* ASCII dump */
            char sym = ((char *)(ptr + i))[j];
			xputc((unsigned char)((sym >= ' ' && sym <= '~') ? sym : '.'));
		}
        xprintf("\n");
    }
}

FRESULT file_read(const char *path, char *buff, uint16_t buff_size, uint32_t offset) {
    UINT read_count = 0;
    res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
    if(res != FR_OK){
        f_close(&file);
        return res;
    }
    while(f_size(&file) > offset){
        memset(buff, 0, buff_size);
        res = f_lseek(&file, offset);
        if(res != FR_OK) break;
        res = f_read(&file, (void *)(buff), buff_size, &read_count);
        if(res != FR_OK) break;
        for(uint16_t i = 0; i < read_count; i++){
            char val = buff[i];
            xputc(val);
            if(val == '\n'){
                xputc('\r');
            }
        }
        offset += read_count;
    }
    f_close(&file);
    return res;
}

FRESULT copy_to_flash(const char *path, char *buff, uint16_t buff_size, uint32_t addr) {
    UINT read_count = 0;
    uint32_t offset = 0;
    res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
    if(res != FR_OK){
        f_close(&file);
        return res;
    }
    while(f_size(&file) > offset){
        memset(buff, 0, buff_size);
        res = f_lseek(&file, offset);
        if(res != FR_OK) break;
        res = f_read(&file, (void *)(buff), buff_size, &read_count);
        if(res != FR_OK) break;
        if(FLASH_write(addr + offset, (uint64_t *)buff, (uint16_t)(read_count / 8)) != 0){
            res = FR_INT_ERR;
            break;
        }
        offset += read_count;
    }
    f_close(&file);
    return res;
}

FRESULT tail(const char *path, char *buff, size_t buff_size, uint32_t str_count, uint32_t *offset){
    uint32_t counter = 0;
    UINT read_count = 0;
    int32_t read_ptr;
    res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
    if(res != FR_OK){
        f_close(&file);
        return res;
    }
    read_ptr = f_size(&file);
    while(read_ptr > 0){
        memset(buff, 0, buff_size);
        read_ptr -= buff_size;
        if(read_ptr < 0){
            read_ptr = 0;
        }
        res = f_lseek(&file, (uint32_t)read_ptr);
        if(res != FR_OK) break;
        res = f_read(&file, (void *)(buff), buff_size, &read_count);
        if(res != FR_OK) break;
        for(uint32_t i = buff_size; i > 0; i--){
            if(buff[i] == '\n'){
                counter++;
                if(counter >= str_count + 1){
                    *offset = (uint32_t)read_ptr + i + 1;
                    f_close(&file);
                    return res;
                }
            }
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
            print_help();  // print help
            break;
        } else if (strcmp(argv[i], _CMD_CLEAR) == 0) {
            xprintf("\033[2J\033[H");  // ESC seq for clear entire screen
            break;
        } else if (strcmp(argv[i], _CMD_ECHO) == 0) {
            if (argc > 1) {
                for (i = 1; i < argc; i++) {
                    xprintf("%c ", argv[i]);
                }
                xprintf("\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_TASKS) == 0) {
            if (argc > 1) {
                xprintf("reset cmd does not accept any arguments\n");
            }
            show_monitor();
            break;
        } else if (strcmp(argv[i], _CMD_CURR_BLOCK) == 0) {
            if (argc > 1) {
                xprintf("get_curr_block cmd does not accept any arguments\n");
            }
            xprintf("%u\n", HaveRunFlashBlockNum());
            break;
        } else if (strcmp(argv[i], _CMD_NEOFETCH) == 0) {
            if(argc == 1){
                neofetch();
            }
            break;
        } else if (strcmp(argv[i], _CMD_PREF_BLOCK) == 0) {
            if (argc == 1) {
                xprintf("%u\n", HavePrefFlashBlockNum());
            } else if (argc == 2) {
                long block_num;
                xatoi((char **)(&argv[1]), &block_num);
                FLASH_status status = SetPrefferedBlockNum((uint8_t)block_num);
                if (status != FLASH_OK) {
                    xprintf("failed to set preffered block\n");
                }
                system_config.pref_block = (uint64_t)HavePrefFlashBlockNum();
            } else {
                xprintf("pref_block cmd accepts only one argument\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_CHECK_CRC) == 0) {
            if (argc == 2) {
                long block_num;
                xatoi((char **)(&argv[1]), &block_num);
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
        } else if (strcmp(argv[i], _CMD_BEEP) == 0) {
            if (argc == 3) {
                long freq;
                long duration;
                xatoi((char **)(&argv[1]), &freq);
                xatoi((char **)(&argv[2]), &duration);
                BUZZ_beep(&buzzer, (uint16_t)freq, (uint16_t)duration);
            } else {
                xprintf("you should set only frequency and duration\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_ADC_MEASURE) == 0) {
            if(argc == 1){
                gpio_state(EN_PERIPH, LOW);
                ADC_Start(&adc);
                ADC_WaitMeasures(&adc, 1000000);
                gpio_state(EN_PERIPH, HIGH);
                for(uint8_t i = 0; i < 11; i++){
                    xprintf("A%d: %d\n", i + 1, adc.reg_channel_queue[i].result_mv);
                }
                xprintf("VREF: %d\n", adc.reg_channel_queue[11].result_mv);
            } else {
                xprintf("Too many arguments\n");
            }
        } else if (strcmp(argv[i], _CMD_TEMPS_MEASURE) == 0) {
            if(argc == 1){
                gpio_state(EN_PERIPH, LOW);
                if(!sensors_bus.is_calibrated){
                    xprintf("Start measure uncalibrated sensors\n");
                }
                OW_Status status = OW_OK;
                if(sensors_bus.found_amount == 0){
                    xprintf("Start searching sensors\n");
                    status = OW_SearchDevices(sensors_bus.ow, &sensors_bus.found_amount);
                    if(status != OW_OK){
                        if(status == OW_EMPTY_BUS){
                            xprintf("Not found connected sensors. Check the power supply.\n");
                        } else if(status == OW_ROM_FINDING_ERROR){
                            xprintf("OneWire reading error\n");
                        } else if(status == OW_TIMEOUT){
                            xprintf("OneWire TIMEOUT\n");
                        }
                        gpio_state(EN_PERIPH, HIGH);
                        break;
                    }
                    for(uint8_t i = 0; i < sensors_bus.connected_amount; i++){
                        // initial_serials[i] = sensors_bus.ow->ids[i].serial_code;
                        sensors_bus.sensors[i].serialNumber = &(sensors_bus.ow->ids[i]);
                    }
                    if(status == OW_OK && sensors_bus.found_amount > 0){
                        xprintf("Founded sensors: %d\n", sensors_bus.found_amount + 1);
                    } else {
                        if(status == OW_EMPTY_BUS){
                            xprintf("Not found connected sensors. Check the power supply.\n");
                        } else if(status == OW_ROM_FINDING_ERROR){
                            xprintf("OneWire reading error\n");
                        } else if(status == OW_TIMEOUT){
                            xprintf("OneWire TIMEOUT\n");
                        }
                        gpio_state(EN_PERIPH, HIGH);
                        break;
                    }
                }
                status = TemperatureSensorsMeasure(&sensors_bus, sensors_bus.is_calibrated);
                if(status != OW_OK){
                    if(status == OW_EMPTY_BUS){
                        xprintf("Not found connected sensors. Check the power supply.\n");
                    } else if(status == OW_ROM_FINDING_ERROR){
                        xprintf("OneWire reading error\n");
                    } else if(status == OW_TIMEOUT){
                        xprintf("OneWire TIMEOUT\n");
                    }
                    gpio_state(EN_PERIPH, HIGH);
                    break;
                } else {
                    gpio_state(EN_PERIPH, HIGH);
                    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
                        xprintf("T%02d [0x%016llX] : %.2f °С\n", i + 1,
                                sensors_bus.sensors[i].serialNumber->serial_code,
                                sensors_bus.sensors[i].temperature);
                    }
                }
            }
            break;
        } else if (strcmp(argv[i], _CMD_CALIB_SENSORS) == 0) {
            if(argc == 1){
                if(sensors_bus.is_calibrated){
                    xprintf("Temperature sensors already calibrated."\
                            "Recalibration process started\n");
                }
                gpio_state(EN_PERIPH, LOW);
                DS18B20_BUS_Status status = Calibration_routine(&sensors_bus);
                if(status != (DS18B20_BUS_Status)OW_OK){
                    xprintf("Calibration failed: %d\n", status);
                    gpio_state(EN_PERIPH, HIGH);
                    break;
                }
                for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
                    xprintf("T%d: %08lX\n", i, sensors_bus.serials[i]);
                }
                if(system_config.auto_save_config){
                    FLASH_status result = save_system_config(&system_config);
                    if(result != FLASH_OK)
                        xprintf("Saving configuration failed!\n");
                }
                gpio_state(EN_PERIPH, HIGH);
            }
            break;
        } else if (strcmp(argv[i], _CMD_ACTION_MODE) == 0) {
            if(argc == 1){
                xprintf(system_config.action_mode == 1 ? "cli\n" : "auto\n");
            } else if(argc == 2){
                if(strcmp(argv[1], "cli") == 0){
                    if(system_config.action_mode != 0){
                        system_config.action_mode = 0;
                        FLASH_status result = save_system_config(&system_config);
                        if(result != FLASH_OK)
                            xprintf("Saving configuration failed!\n");
                    }
                } else if(strcmp(argv[1], "auto") == 0){
                    if(!sensors_bus.is_calibrated){
                        xprintf("For going to autonomous mode you need to "\
                                "calibrate temperature sensors\n");
                        break;
                    }
                    if(system_config.action_mode != 1){
                        system_config.action_mode = 1;
                        FLASH_status result = save_system_config(&system_config);
                        if(result != FLASH_OK)
                            xprintf("Saving configuration failed!\n");
                    }
                } else {
                    xprintf("Incorrect argument\n");
                }
            } else {
                xprintf("Too many arguments\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_SAVE_CONFIG) == 0) {
            if(argc <= 2){
                if(argc == 2){
                    long autosave_state = 2;
                    xatoi((char **)&argv[1], &autosave_state);
                    if(autosave_state == 0 || autosave_state == 1){
                        system_config.auto_save_config = autosave_state;
                    } else {
                        xprintf("Incorrect argument\n");
                        break;
                    }
                }
                FLASH_status result = save_system_config(&system_config);
                if(result != FLASH_OK)
                    xprintf("Saving configuration failed!\n");
            } else {
                xprintf("Too many arguments\n");
            }
        } else if (strcmp(argv[i], _CMD_SLEEP) == 0) {
            if(argc == 2){
                long sleep_time = 0;
                xatoi((char **)&argv[1], &sleep_time);
                if(sleep_time < 1 || sleep_time > 0xFFFF){
                    xprintf("Incorrect sleep time argument\n");
                } else {
                    RTC_auto_wakeup_enable((uint16_t)sleep_time);
                    stop_cortex();
                }
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
                    xprintf("FAILED\n");
                } else {
                    f_getcwd(curr_dir_name + 26, 91);
                    rl.prompt_len = strlen(curr_dir_name) - (27 - _PROMPT_LEN);
                    memcpy(curr_dir_name + strlen(curr_dir_name), "\033[37m$\033[0m ", 11);
                    rl.prompt_str = curr_dir_name;
                }
            } else {
                xprintf("You need to set only \"file_name\" argument\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_RENAME) == 0) {
            if(argc == 3){
                if(f_rename(argv[1], argv[2]) != FR_OK){
                    xprintf("Failed\n");
                }
            } else {
                xprintf("Incorrect arguments for \'rename\'\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_RM) == 0) {
            if(argc == 2){
                if (f_unlink(argv[1]) != FR_OK) {
                    xprintf("FAILED\n");
                }
            } else {
                xprintf("You need to set only \"file_name\" argument\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_CAT) == 0) {
            if(argc == 2){
                if(file_read(argv[1], file_buff, 512, 0) != FR_OK){
                    xprintf("Failed\n");
                }
            }
            break;
        } else if (strcmp(argv[i], _CMD_TAIL) == 0) {
            if(argc == 3){
                uint32_t offset = 0;
                long str_count = 0;
                xatoi((char **)(&argv[2]), &str_count);
                if(str_count > 0){
                    if(tail(argv[1], file_buff, 512, str_count, &offset) == FR_OK){
                        if(file_read(argv[1], file_buff, 512, offset) != FR_OK){
                            xprintf("Failed\n");
                        }
                    } else {
                        xprintf("Failed\n");
                    }
                }
            } else {
                xprintf("Incorrect arguments\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_DUMP_MEM) == 0) {
            if(argc == 3){
                long addr = 0;
                long len = 0;
                xatoi((char **)&argv[1], &addr);
                xatoi((char **)&argv[2], &len);
                if(addr < 0x8000000 && addr > 0x803FFFF){
                    xprintf("Address out of range\n");
                }
                addr -= MOD(addr, 4);
                dump_memory(addr, (uint32_t *)addr, (size_t)len);
            }
            break;
        } else if (strcmp(argv[i], _CMD_TOUCH) == 0) {
            if(argc == 2){
                if (f_open(&file, argv[1], FA_OPEN_ALWAYS | FA_READ) != FR_OK) {
                    xprintf("FAILED\n");
                } else {
                    f_close(&file);
                }
            } else {
                xprintf("You need to set only \"file_name\" argument\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_MKDIR) == 0) {
            if(argc == 2){
                if(f_mkdir(argv[1]) != FR_OK){
                    xprintf("FAILED\n");
                }
            } else {
                xprintf("You need to set only \"dir_name\" argument\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_RESET) == 0) {
            if (argc == 1) {
                __NVIC_SystemReset();
            } else {
                xprintf("reset cmd does not accept any arguments\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_TIME) == 0) {
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
                xprintf("%s\n", rtc_buffer);
            } else {
                xprintf("incorrect arguments for \'time\' cmd\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_RADIO_CONF) == 0) {
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
                xprintf("you should set only frequency and duration\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_SEND_RADIO) == 0) {
            if (argc == 2) {
                LoRa_transmit(&sx127x, (uint8_t *)argv[1], strlen(argv[1]));
            } else {
                xprintf("you should set only message string for transaction\n");
            }
            break;
        } else if (strcmp(argv[i], _CMD_XMODEM) == 0) {
            if (argc != 1) {
                xprintf("xmodem cmd does not accept any arguments\n");
                break;
            }
            uint32_t write_addr = HaveRunFlashBlockNum() != 0 ? MAIN_FW_ADDR : RESERVE_FW_ADDR;
            xmodem_receive(write_addr);
            break;

        } else if (strcmp(argv[i], _CMD_ERASE_FIRMWARE) == 0) {
            if (argc != 1) {
                xprintf("erase_firmware cmd does not accept any arguments\n");
                break;
            }
            FLASH_status status = FLASH_erase_firmware(HaveRunFlashBlockNum());
            if(status != FLASH_OK) xprintf("Failed\n");
        } else if (strcmp(argv[i], _CMD_UPLOAD_SD_FW) == 0) {
            if (argc == 2) {
                uint32_t write_addr = HaveRunFlashBlockNum() != 0 ? MAIN_FW_ADDR : RESERVE_FW_ADDR;
                xprintf("Writing to address %08lX\n", write_addr);
                if(copy_to_flash(argv[1], file_buff, 512, write_addr) != FR_OK){
                    xprintf("Failed\n");
                }
            }
        } else {
            xprintf("Incorrect arguments\n");
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
void sigint(void) { xprintf("^C catched!\n"); }