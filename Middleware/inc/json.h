// Copyright (c) 2023 Cesanta Software Limited
// SPDX-License-Identifier: AGPL-3.0 or commercial

#ifndef STR_H_
#define STR_H_

#include <stddef.h>



// JSON parsing API
int json_get(const char *buf, int len, const char *path, int *size);
int json_get_num(const char *buf, int len, const char *path, long *val);
int json_get_big_num(const char *buf, int len, const char *path, long long *val);
int json_set_num(char *buf, int len, const char *path, long val);
int json_get_bool(const char *buf, int len, const char *path, int *val);
int json_get_str(const char *buf, int len, const char *path, char *dst, size_t dlen);




#endif  // STR_H_