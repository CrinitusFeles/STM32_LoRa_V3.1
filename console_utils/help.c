#include "config.h"


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
      );
}