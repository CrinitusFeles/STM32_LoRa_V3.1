#ifndef STRING_UTILS_H_
#define STRING_UTILS_H_

#include "stm32l4xx.h"

int convert_file_name(char *filename, char *converted_name);
uint8_t compare_short_file_names(char *fname1, char *fname2);

#endif