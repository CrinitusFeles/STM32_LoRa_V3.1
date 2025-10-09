#include "stm32l4xx.h"

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