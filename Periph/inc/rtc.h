#ifndef CODE_HEADER_RTC_H_
#define CODE_HEADER_RTC_H_

#include "stm32l4xx.h"

typedef struct s_RTC_struct_full {
	uint8_t year_tens;		// 20[1]9
	uint8_t year_units;		// 201[9]
	uint8_t week_day;		// 001 for Monday, 111 for Sunday
	uint8_t month_tens;		// [1]2
	uint8_t month_units;		// 1[2]
	uint8_t date_tens;		// [2]5
	uint8_t date_units;		// 2[5]
	uint8_t hour_tens;		// [0]0
	uint8_t hour_units;		// 0[0]
	uint8_t minute_tens;		// [0]3
	uint8_t minute_units;		// 0[3]
	uint8_t second_tens;		// [1]5
	uint8_t second_units;		// 1[5]
} RTC_struct_full;

typedef struct s_RTC_struct_brief {
	uint8_t years;
	uint8_t week_day;
	uint8_t months;
	uint8_t date;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
} RTC_struct_brief;


void rtc_writeToBkp(uint32_t *val, uint8_t size);
void write_single_bkp_reg(uint8_t reg_num, uint32_t val);
uint32_t RTC_EncodeDateTime(RTC_struct_brief *dt);
void RTC_time_difference(RTC_struct_brief *older_time, RTC_struct_brief *newer_time, RTC_struct_brief *difference);
void RTC_auto_wakeup_enable(uint16_t period_sec);
void RTC_get_time(RTC_struct_brief *br_data);
void RTC_data_update(RTC_struct_brief *br_data);
void RTC_set_date(uint32_t date_reg);
void RTC_set_time(uint32_t time_reg);
uint16_t RTC_string_datetime(RTC_struct_brief *rtc_data, char *str);
uint32_t RTC_struct_brief_time_converter(RTC_struct_brief *br_data);
uint32_t RTC_struct_brief_date_converter(RTC_struct_brief *br_data);
uint8_t RTC_Init();
#endif /* CODE_HEADER_RTC_H_ */
