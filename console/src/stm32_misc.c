#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stm32_misc.h"
#include "../inc/config.h"
#include "buzzer.h"
#include "monitor_task.h"
#include "xmodem.h"
#include "flash.h"
#include "system_select.h"
#include "rtc.h"


#define _STM_DEMO_VER "1.0"

// definition commands word
#define _CMD_HELP                   "help"              // 1
#define _CMD_CLEAR                  "clear"             // 2
#define _CMD_CLR                    "clear_port"        // 3
#define _CMD_SET                    "set_port"          // 4
#define _CMD_ECHO                   "echo"              // 5
#define _CMD_BEEP                   "beep"              // 6
#define _CMD_RESET                  "reset"             // 7
#define _CMD_TASKS                  "tasks"             // 8
#define _CMD_XMODEM                 "xmodem"            // 9
#define _CMD_SET_PREF_BLOCK         "set_pref_block"    // 10
#define _CMD_GET_PREF_BLOCK         "get_pref_block"    // 11
#define _CMD_GET_CURR_BLOCK         "get_curr_block"    // 12
#define _CMD_ERASE_FIRMWARE         "erase_firmware"    // 13
#define _CMD_CHECK_CRC              "check_crc"         // 14
#define _CMD_SEND_RADIO             "send_radio"        // 15
#define _CMD_SET_TIME               "set_time"          // 16
#define _CMD_MAKE_MEASURE           "make_measure"      // 17
#define _CMD_SLEEP                  "sleep"             // 18
#define _CMD_GET_TIME               "get_time"          // 19


#define _NUM_OF_CMD 19
#define _NUM_OF_SETCLEAR_SCMD 2

// available  commands
char *keyworld[] = {_CMD_HELP, _CMD_CLEAR, _CMD_ECHO, _CMD_SET, _CMD_CLR,
                    _CMD_BEEP, _CMD_RESET, _CMD_TASKS, _CMD_XMODEM,
                    _CMD_SET_PREF_BLOCK, _CMD_GET_PREF_BLOCK, _CMD_GET_CURR_BLOCK,
                    _CMD_SET_TIME, _CMD_ERASE_FIRMWARE, _CMD_CHECK_CRC,
                    _CMD_SEND_RADIO, _CMD_MAKE_MEASURE, _CMD_SLEEP,
                    _CMD_GET_TIME};
// 'set/clear' command argements

// array for comletion
char *compl_world[_NUM_OF_CMD + 1];

void put_char(unsigned char ch);

//*****************************************************************************
void init(void);

void print (const char * str){
	printf(str);
}

//*****************************************************************************
void print_help(void) {
    print("Use TAB key for completion\n\rCommand:\n\r");
    print("  clear                          - clear screen\n\r");
    print("  echo                           - print arg to screen\n\r");
    print("  reset                          - reset MCU\n\r");
    print("  tasks                          - show tasks\n\r");
    print("  xmodem                         - start flashing procedure over XModem protocol\n\r");
    print("  set_pref_block [block_num]     - set firmware preffered block\n\r");
    print("  get_pref_block                 - get firmware preffered block\n\r");
    print("  get_curr_block                 - get current firmware block\n\r");
    print("  erase_firmware                 - erase firmware \n\r");
    print("  check_crc [block_num]          - check firmware CRC \n\r");
    print("  send_radio [sf] [bw] [*data]   - send LoRa package \n\r");
    print("  set_time [year] [month] [day] [hours] [mins] [seconds] - sets MCU RTC\n\r");
    print("  get_time                       - MCU RTC timestring\n\r");
    print("  beep [freq] [duration]         - make beep signal certain frequency and duration\n\r");
    print("  sleep [sleep_sec]              - go to sleep mode\n\r");
    print("  set_port [port] [pin]          - set 1 port[pin] value, support only 'port_b' and 'port_d'\n\r");
    print("  clear_port [port] [pin]        - set 0 port[pin] value, support only 'port_b' and 'port_d'\n\r");
}

//*****************************************************************************
// void set_port_val(unsigned char *port, int pin, int val) {
//     if ((*port == PORTD) && (pin < 2) && (pin > 7)) {
//         print("only 2..7 pin avialable for PORTD\n\r");
//         return;
//     }

//     if ((*port == PORTB) && (pin > 5)) {
//         print("only 0..5 pin avialable for PORTB\n\r");
//         return;
//     }

//     if (val) {
//         (*port) |= 1 << pin;
//     } else {
//         (*port) &= ~(1 << pin);
//     }
// }
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
        } else if (strcmp(argv[i], _CMD_CLEAR) == 0) {
            print("\033[2J");  // ESC seq for clear entire screen
            print("\033[H");   // ESC seq for move cursor at left-top corner
        // } else if ((strcmp(argv[i], _CMD_SET) == 0) || (strcmp(argv[i], _CMD_CLR) == 0)) {
        //     if (++i < argc) {
        //         int val = strcmp(argv[i - 1], _CMD_CLR);
        //         unsigned char *port = NULL;
        //         int pin = 0;
        //         // if (strcmp(argv[i], _SCMD_PD) == 0) {
        //         //     port = (unsigned char *)&PORTD;
        //         // } else if (strcmp(argv[i], _SCMD_PB) == 0) {
        //         //     port = (unsigned char *)&PORTB;
        //         // } else {
        //         //     print("only '");
        //         //     print(_SCMD_PB);
        //         //     print("' and '");
        //         //     print(_SCMD_PD);
        //         //     print("' support\n\r");
        //         //     return 1;ghj
        //         // }
        //         // if (++i < argc) {
        //         //     pin = atoi(argv[i]);
        //         //     set_port_val(port, pin, val);
        //         //     return 0;
        //         // } else {
        //         //     print("specify pin number, use Tab\n\r");
        //         //     return 1;
        //         // }
        //     // } else {
        //     //     print("specify port, use Tab\n\r");
        //     //     return 1;
        //     // }
        } else if(strcmp(argv[i], _CMD_ECHO) == 0){
            if (argc > 1) {
                for(i = 1; i < argc; i++){
                    print(argv[i]);
                    print(" ");
                }
                print("\n\r");
            }
        } else if(strcmp(argv[i], _CMD_TASKS) == 0){
            if(argc == 1){
                show_monitor();
                break;
            } else {
                print("reset cmd does not accept any arguments\n\r");
            }
        } else if(strcmp(argv[i], _CMD_GET_PREF_BLOCK) == 0){
            if(argc == 1){
                printf("%u\n\r", HavePrefFlashBlockNum());
                break;
            } else {
                print("get_pref_block cmd does not accept any arguments\n\r");
            }
        } else if(strcmp(argv[i], _CMD_GET_CURR_BLOCK) == 0){
            if(argc == 1){
                printf("%u\n\r", HaveRunFlashBlockNum());
                break;
            } else {
                print("get_curr_block cmd does not accept any arguments\n\r");
            }
        } else if(strcmp(argv[i], _CMD_SET_PREF_BLOCK) == 0){
            if(argc == 2){
                uint8_t status = SetPrefferedBlockNum(atoi(argv[1]));
                if(status){
                    print("failed to set preffered block\n\r");
                }
                break;
            } else {
                print("set_pref_block cmd accepts only one argument\n\r");
            }
        } else if(strcmp(argv[i], _CMD_CHECK_CRC) == 0){
            if(argc == 2){
                uint32_t addr = atoi(argv[1]) == 0 ? 0x8000000 : 0x8010000;
                printf("%d\n\r", CheckBlock((uint32_t *)addr));
                break;
            } else {
                print("check_crc cmd accepts only one argument\n\r");
            }
        } else if(strcmp(argv[i], _CMD_BEEP) == 0){
            if (++i < argc) {
                if(argc == 3){
                    uint16_t freq = atoi(argv[1]);
                    uint16_t duration = atoi(argv[2]);
                    BUZZ_beep(&buzzer, freq, duration);
                } else {
                    print("you should set only frequency and duration\n\r");
                }
                break;
            }
        } else if(strcmp(argv[i], _CMD_RESET) == 0){
            if(argc == 1){
                __NVIC_SystemReset();
            } else {
                print("reset cmd does not accept any arguments\n\r");
            }
        } else if(strcmp(argv[i], _CMD_SET_TIME) == 0){
            if(argc == 7){
                RTC_struct_brief new_time = {
                    .years=atoi(argv[1]),
                    .months=atoi(argv[2]),
                    .date=atoi(argv[3]),
                    .hours=atoi(argv[4]),
                    .minutes=atoi(argv[5]),
                    .seconds=atoi(argv[6]),
                    .sub_seconds=0
                };
                RTC_data_update(&new_time);
                RTC_string_datetime(rtc_buffer);
                print(rtc_buffer);
                print("\n\r");
                break;
            } else {
                print("reset cmd does not accept any arguments\n\r");
            }
        } else if(strcmp(argv[i], _CMD_GET_TIME) == 0){
            if(argc == 1){
                RTC_string_datetime(rtc_buffer);
                print(rtc_buffer);
                print("\n\r");
                break;
            } else {
                print("xmodem cmd does not accept any arguments\n\r");
            }
        } else if(strcmp(argv[i], _CMD_XMODEM) == 0){
            if(argc == 1){
                xmodem_receive(0);
                break;
            } else {
                print("xmodem cmd does not accept any arguments\n\r");
            }
        } else {
            print("command: '");
            print((char *)argv[i]);
            print("' Not found.\n\r");
            break;
        }
        i++;
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