#include "ff.h"

FRESULT res;
FIL file;
char file_buff[FILE_BUFFER] = {0};


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
