#include "console_utils.h"
#include "config.h"
#include "flash.h"
#include "xprintf.h"
#include "system_config.h"
#include "gsm.h"
#include "rtc.h"
#include <string.h>


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
    FRESULT res = f_opendir(&dir, path);

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
    xprintf("%10s | %-8s | %-8s | %-8s | %-8s | %-16s\n", "Address", "0", "4", "8", "C", "ASCII");
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

FRESULT dump_file(const char *path, char *buff, uint16_t buff_size, uint32_t offset) {
    UINT read_count = 0;
    FRESULT res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
    if(res != FR_OK){
        f_close(&file);
        return res;
    }
    xprintf("%10s | %-8s | %-8s | %-8s | %-8s | %-16s\n", "Address", "0", "4", "8", "C", "ASCII");
    while(f_size(&file) > offset){
        memset(buff, 0, buff_size);
        res = f_lseek(&file, offset);
        if(res != FR_OK) break;
        res = f_read(&file, (void *)(buff), buff_size, &read_count);
        if(res != FR_OK) break;

        for(uint32_t i = 0; i < read_count; i += 16){
            xprintf("0x%08lX | ", offset + i);
            for(uint8_t c = 0; c < 16; c+=4){
                uint32_t val = *(uint32_t*)(buff + i + c);
                if(val == 0){
                    xprintf("\033[36m");
                } else if (val == 0xFFFFFFFF){
                    xprintf("\033[33m");
                } else if ((uint16_t)(val) == 0x0FF1){
                    xprintf("\033[45m");
                }
                xprintf("%08lX", __REV(val));
                if(val == 0 || val == 0xFFFFFFFF || (uint16_t)(val) == 0x0FF1){
                    xprintf("\033[0m");
                }
                xprintf(" | ");
            }
            for (uint8_t j = 0; j < 16; j++) {		/* ASCII dump */
                char sym = ((char *)(buff + i))[j];
                xputc((unsigned char)((sym >= ' ' && sym <= '~') ? sym : '.'));
            }
            xprintf("\n");
        }

        offset += read_count;
    }
    xputc('\n');
    f_close(&file);
    return res;
}

FRESULT file_read(const char *path, char *buff, uint16_t buff_size, uint32_t offset) {
    UINT read_count = 0;
    FRESULT res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
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
            if(buff[i] < 31 && (buff[i] != '\n' && buff[i] != '\r')){
                buff[i] = 0;
            }
            char val = buff[i];
            xputc(val);
            if(val == '\n'){
                xputc('\r');
            }
        }
        offset += read_count;
    }
    xputc('\n');
    f_close(&file);
    return res;
}

FRESULT copy_to_flash(const char *path, char *buff, uint16_t buff_size, uint32_t addr) {
    UINT read_count = 0;
    uint32_t offset = 0;
    FRESULT res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
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
    FRESULT res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
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

FRESULT head(const char *path, char *buff, size_t buff_size, uint32_t str_count){
    UINT read_count = 0;
    uint32_t read_count_sum = 0;
    FRESULT res = f_open(&file, path, FA_OPEN_EXISTING | FA_READ);
    if(res != FR_OK){
        f_close(&file);
        return res;
    }
    while(read_count_sum < f_size(&file) && str_count){
        memset(buff, 0, buff_size);
        res = f_lseek(&file, read_count_sum);
        if(res != FR_OK) break;
        res = f_read(&file, (void *)(buff), buff_size, &read_count);
        if(res != FR_OK) break;
        for(uint16_t i = 0; i < read_count; i++){
            char val = buff[i];
            xputc(val);
            if(val == '\n'){
                str_count--;
                xputc('\r');
                if(str_count == 0) break;
            }
        }
        read_count_sum += read_count;
    }
    f_close(&file);
    return res;
}


void _log_error(char *msg){
    uint16_t sd_ptr = 0;
    UINT _written_count = 0;
    f_open(&file, "errors.log", FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    f_lseek(&file, file.obj.objsize);
    sd_ptr += RTC_string_datetime(file_buff + sd_ptr);
    sd_ptr += xsprintf(file_buff + sd_ptr, "  ");
    sd_ptr += xsprintf(file_buff + sd_ptr, msg);
    f_write(&file, file_buff, strlen(file_buff), &_written_count);
    f_close(&file);
}

bool GSM_SendFile(GSM *driver, char *filename, uint32_t read_amount){
    UINT read_count = 0;
    FRESULT res = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
    if(res != FR_OK){
        f_close(&file);
        _log_error("Failed to read SD measures\n");
        return false;
    }
    if(read_amount > file.obj.objsize)
        read_amount = file.obj.objsize;
    while(f_size(&file) > file.obj.objsize - read_amount){
        res = f_lseek(&file, file.obj.objsize - read_amount);
        if(res != FR_OK) break;
        res = f_read(&file, (void *)(file_buff), FILE_BUFFER, &read_count);
        if(res != FR_OK) break;
        if(GSM_SendTCP(driver, file_buff, read_count) == 0){
            f_close(&file);
            _log_error("Failed send data to server\n");
            return false;
        }
        read_amount -= read_count;
    }
    f_close(&file);
    return true;
}