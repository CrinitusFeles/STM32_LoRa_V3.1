#ifndef INC_CONSOLE_UTILS_H_
#define INC_CONSOLE_UTILS_H_

#include "stm32l4xx.h"
#include "ff.h"
#include "gsm.h"
#include <stdlib.h>
#include <stdbool.h>

void neofetch();
void ls(const char *path);
void dump_memory(unsigned long addr, uint32_t *ptr, size_t len);
FRESULT dump_file(const char *path, char *buff, uint16_t buff_size, uint32_t offset);
FRESULT file_read(const char *path, char *buff, uint16_t buff_size, uint32_t offset);
FRESULT copy_to_flash(const char *path, char *buff, uint16_t buff_size, uint32_t addr);
FRESULT tail(const char *path, char *buff, size_t buff_size, uint32_t str_count, uint32_t *offset);
FRESULT head(const char *path, char *buff, size_t buff_size, uint32_t str_count);
bool GSM_SendFile(GSM *driver, char *filename, uint32_t read_amount);
#endif