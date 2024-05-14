/*
 * rtc.c
 *
 *  Created on: 3 февр. 2020 г.
 *      Author: BreakingBad
 */
#include "rtc.h"
#include "xprintf.h"

/*
 * 						Effect of low-power modes on RTC
 * 		 _____________________________________________________________________________________________________
 * 		|	Sleep			 |	No effect. RTC interrupts cause the device to exit the Sleep mode.			  |
 * 		|	Low-power run	 |	No effect.																	  |
		|	Low-power sleep  |	No effect. RTC interrupts cause the device to exit the Low-power sleep mode	  |
		|____________________|________________________________________________________________________________|
 * 		|	Stop 0			 |																				  |
 * 		|	Stop 1			 |	Peripheral registers content is kept.										  |
 * 		|___Stop 2___________|________________________________________________________________________________|
 * 		|	Standby			 |	The RTC remains active when the RTC clock source is LSE or LSI. RTC alarm,	  |
		|					 |	RTC tamper event, RTC timestamp event, and RTC Wakeup cause the device to	  |
		|					 |	exit the Standby mode.														  |
 * 		|	Shutdown		 |	The RTC remains active when the RTC clock source is LSE. RTC alarm, RTC		  |
		|					 |	tamper event, RTC timestamp event, and RTC Wakeup cause the device to exit	  |
		|					 |	the Shutdown mode.															  |
 * 		|____________________|________________________________________________________________________________|
 *
 *
 */


void RTC_auto_wakeup_enable(uint16_t period_sec) {
	// unlock write protection
    PWR->CR1 |= PWR_CR1_DBP;
	while((RTC->ISR & RTC_ISR_RSF) == 0);
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

	RTC->CR &= ~RTC_CR_WUTE;  // disable the wakeup timer
	// polling to make sure the access to wakeup auto-reload counter and to WUCKSEL[2:0] bits is allowed
	while (!(RTC->ISR & RTC_ISR_WUTWF));
	// program the wakeup auto-reload value and the wakeup clock selection
	RTC->WUTR = period_sec;	// the WUTF flag is set every (WUT[15:0] + 1) = (0 + 1) = (1) ck_wut cycles
	RTC->CR &= ~RTC_CR_WUCKSEL_1;	// 10x: ck_spre (usually 1 Hz) clock is selected
	RTC->CR |= RTC_CR_WUCKSEL_2;

	// enable the RTC Wakeup interrupt - enable the EXTI Line 18
	if (!(RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN)) RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	EXTI->IMR1 |= EXTI_IMR1_IM20;	// interrupt request mask - IM22 is not masked now
	EXTI->RTSR1 |= EXTI_RTSR1_RT20;	// rising edge trigger enabled for EXTI line 17

	NVIC_EnableIRQ(RTC_WKUP_IRQn);	// enable the RTC_WKUP IRQ channel in the NVIC
	NVIC_ClearPendingIRQ(RTC_WKUP_IRQn);
	NVIC_SetPriority(RTC_WKUP_IRQn, 0);	// highest priority
    if(RTC->ISR & RTC_ISR_WUTF) RTC->ISR &= ~RTC_ISR_WUTF;
	RTC->CR |= RTC_CR_WUTIE;  // 1: Wakeup timer interrupt enabled
	RTC->CR |= RTC_CR_WUTE;  // enable the timer again

	// lock write protection - writing a wrong key reactivates the write protection
	RTC->WPR = 0xFF;
	__enable_irq();	// global interrupts enable
}

void RTC_set_time(uint32_t time_reg){
	// uint32_t time_value, date_value;
	// unlock write protection
    PWR->CR1 |= PWR_CR1_DBP;
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;
	// initialization mode on (INITF == 1) - calendar counter is stopped, can update now
	RTC->ISR |= RTC_ISR_INIT;
	while (!(RTC->ISR & RTC_ISR_INITF));  // INITF polling
	RTC->TR = time_reg;
	RTC->ISR &= ~RTC_ISR_INIT;  // exit from the init mode
	// lock write protection - writing a wrong key reactivates the write protection
	RTC->WPR = 0xFF;
    PWR->CR1 &= ~PWR_CR1_DBP;
}


uint32_t RTC_struct_brief_time_converter(RTC_struct_brief *br_data){
    uint32_t time_value = 0;
    time_value |= ((br_data->hours / 10) << RTC_TR_HT_Pos) | ((br_data->hours % 10) << RTC_TR_HU_Pos);
    time_value |= ((br_data->minutes / 10) << RTC_TR_MNT_Pos) | ((br_data->minutes % 10) << RTC_TR_MNU_Pos);
    time_value |= ((br_data->seconds / 10) << RTC_TR_ST_Pos) | ((br_data->minutes % 10) << RTC_TR_SU_Pos);
    return time_value;
}

uint16_t RTC_string_datetime(char *buf){
    // fills first 20 bytes
    int16_t timeout = 10000;
    while (!(RTC->ISR & RTC_ISR_RSF) && --timeout > 0){};	//  Calendar shadow registers synchronized

	uint32_t TR_buf = 0, DR_buf = 0;
    uint16_t prediv_s = RTC->PRER & 0x7FFF;

	TR_buf = (RTC->TR);
	DR_buf = (RTC->DR);
    TR_buf = (RTC->TR);
	DR_buf = (RTC->DR);
    TR_buf = (RTC->TR);
	DR_buf = (RTC->DR);

	uint8_t months = ((((DR_buf & RTC_DR_MT) >> RTC_DR_MT_Pos) * 10) + ((DR_buf & RTC_DR_MU) >> RTC_DR_MU_Pos));
	uint8_t date = ((((DR_buf & RTC_DR_DT) >> RTC_DR_DT_Pos) * 10) + ((DR_buf & RTC_DR_DU) >> RTC_DR_DU_Pos));
    uint16_t y = 2000 + ((((DR_buf & RTC_DR_YT) >> RTC_DR_YT_Pos) * 10) + ((DR_buf & RTC_DR_YU) >> RTC_DR_YU_Pos));
    uint8_t m = months == 0 ? 1 : months;
    uint8_t d = date == 0 ? 1 : date;
    uint8_t h = ((((TR_buf & RTC_TR_HT) >> RTC_TR_HT_Pos) * 10) + ((TR_buf & RTC_TR_HU) >> RTC_TR_HU_Pos));
    uint8_t mm = ((((TR_buf & RTC_TR_MNT) >> RTC_TR_MNT_Pos) * 10) + ((TR_buf & RTC_TR_MNU) >> RTC_TR_MNU_Pos));
    uint8_t s = ((((TR_buf & RTC_TR_ST) >> RTC_TR_ST_Pos) * 10) + ((TR_buf & RTC_TR_SU) >> RTC_TR_SU_Pos));
    uint32_t ss = (prediv_s - RTC->SSR) * 1000 / (prediv_s + 1);
    if(RTC->SSR > prediv_s){
        if(s > 0) s--;
        else {
            s = 59;
            if(mm > 0) m--;
            else{
                mm = 59;
                if(h > 0) h--;
                else h = 23;
            }
        }
    }
    return xsprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d", y, m, d, h, mm, s, ss);
}

uint32_t RTC_struct_brief_date_converter(RTC_struct_brief *br_data){
    uint32_t date_value = 0;
	date_value |= ((br_data->years / 10) << RTC_DR_YT_Pos) | ((br_data->years % 10) << RTC_DR_YU_Pos);
    date_value |= ((br_data->week_day) << RTC_DR_WDU_Pos);
    date_value |= ((br_data->months / 10) << RTC_DR_MT_Pos) | ((br_data->months % 10) << RTC_DR_MU_Pos);
    date_value |= ((br_data->date / 10) << RTC_DR_DT_Pos) | ((br_data->date % 10) << RTC_DR_DU_Pos);
    return date_value;
}

void RTC_set_date(uint32_t date_reg){
	// uint32_t time_value, date_value;
	// unlock write protection
    PWR->CR1 |= PWR_CR1_DBP;
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;
	// initialization mode on (INITF == 1) - calendar counter is stopped, can update now
	RTC->ISR |= RTC_ISR_INIT;
	while (!(RTC->ISR & RTC_ISR_INITF));  // INITF polling
	RTC->DR = date_reg;
	RTC->ISR &= ~RTC_ISR_INIT;  // exit from the init mode
	// lock write protection - writing a wrong key reactivates the write protection
	RTC->WPR = 0xFF;
    PWR->CR1 &= ~PWR_CR1_DBP;
}

void RTC_data_update(RTC_struct_brief *br_data){
	uint32_t time_value = 0, date_value = 0;
	// unlock write protection
	PWR->CR1 |= PWR_CR1_DBP;
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;
	// initialization mode on (INITF == 1) - calendar counter is stopped, can update now
	RTC->ISR |= RTC_ISR_INIT;
	while (!(RTC->ISR & RTC_ISR_INITF));  // INITF polling
	// prescalers - two separate write access - synch predivider
    time_value |= ((br_data->hours / 10) << RTC_TR_HT_Pos) | ((br_data->hours % 10) << RTC_TR_HU_Pos);
    time_value |= ((br_data->minutes / 10) << RTC_TR_MNT_Pos) | ((br_data->minutes % 10) << RTC_TR_MNU_Pos);
    time_value |= ((br_data->seconds / 10) << RTC_TR_ST_Pos) | ((br_data->minutes % 10) << RTC_TR_SU_Pos);
	RTC->TR = time_value;
	date_value |= ((br_data->years / 10) << RTC_DR_YT_Pos) | ((br_data->years % 10) << RTC_DR_YU_Pos);
    date_value |= ((br_data->week_day) << RTC_DR_WDU_Pos);
    date_value |= ((br_data->months / 10) << RTC_DR_MT_Pos) | ((br_data->months % 10) << RTC_DR_MU_Pos);
    date_value |= ((br_data->date / 10) << RTC_DR_DT_Pos) | ((br_data->date % 10) << RTC_DR_DU_Pos);
    RTC->DR = date_value;
	RTC->CR &= ~RTC_CR_FMT;  // 24h format == 0
	RTC->ISR &= ~RTC_ISR_INIT;  // exit from the init mode
	// lock write protection - writing a wrong key reactivates the write protection
	RTC->WPR = 0xFF;
	PWR->CR1 &= ~PWR_CR1_DBP;
}
// uint32_t RTC_struct_brief_to_seconds(RTC_struct_brief *br_data){
//     const uint8_t day_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

//     uint32_t sec = br_data->seconds + br_data->minutes * 60 + br_data->hours * 3600 + br_data->date * 86400 +
// }
uint8_t RTC_Init(){
	RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;

	if((RCC->BDCR & RCC_BDCR_RTCEN) == 0){
        uint16_t timeout = 1000;
		// check the INITS status flag in RTC_ISR register to verify if the calendar is already initialized
		if (RTC->ISR & RTC_ISR_INITS)
            return 0;
		// PWR clock on
		PWR->CR1 |= PWR_CR1_DBP;  // enable WRITE - allow access to backup registers (BDCR)
		if (!(RCC->BDCR & RCC_BDCR_RTCEN)) {  // pass only at the first time
			RCC->BDCR |= RCC_BDCR_BDRST;  // software reset - 1: Resets the entire Backup domain
			RCC->BDCR &= ~RCC_BDCR_BDRST;
		}
		RCC->BDCR |= RCC_BDCR_LSEON;  // enable LSE - Low-speed external oscillator
		while (!(RCC->BDCR & RCC_BDCR_LSERDY) && timeout--);  // wait for being ready by polling
        if(timeout == 0) return -1;
		// LSE as clk source - [01] - LSE oscillator clock used as the RTC clock
		RCC->BDCR |= RCC_BDCR_RTCSEL_0;
		RCC->BDCR &= ~RCC_BDCR_RTCSEL_1;
        RCC->BDCR |= RCC_BDCR_RTCEN;  // RTC clock on
		// RTC_auto_wakeup_enable();
		// RTC_data_update(f_data);
			// unlock write protection
        RTC->WPR = 0xCA;
        RTC->WPR = 0x53;
        // initialization mode on (INITF == 1) - calendar counter is stopped, can update now
        RTC->ISR |= RTC_ISR_INIT;
        while (!(RTC->ISR & RTC_ISR_INITF) && timeout--);  // INITF polling
        if(timeout == 0) return -2;
        // Fck_spre = RTCCLK / (PREDIV_S+1)*(PREDIV_A+1)
        RTC->PRER |= 0x7F << 16;  // the operation must be performed in two separate write accesses.
        RTC->PRER |= 0xFF;  // (0x7F+1) * (0xFF + 1) = 32768
        RTC->CR |= RTC_CR_BYPSHAD;
        RTC->TR = 0x00000000;
        RTC->DR = 0x00000000;
        RTC->CR &= ~RTC_CR_FMT;  // 24h format == 0
        RTC->CALR |= 1 << 12;  // enable LPCAL
        RTC->ISR &= ~RTC_ISR_INIT;  // exit from the init mode
        // lock write protection - writing a wrong key reactivates the write protection
        RTC->WPR = 0xFF;
		PWR->CR1 &= ~PWR_CR1_DBP;
        return 1;  // First init
	}
    return 0;  // already inited
}

void rtc_writeToBkp(uint32_t *val, uint8_t size){
	if(!(RCC->APB1ENR1 & RCC_APB1ENR1_PWREN))
		RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
	PWR->CR1 |= PWR_CR1_DBP;                     //Разрешить доступ к Backup области
	while((RTC->ISR & RTC_ISR_RSF) == 0);        //Wait for RTC APB registers synchronisation
	RTC->WPR = 0xCA;                             //Unlock write protection
	RTC->WPR = 0x53;                             //Unlock write protection

	for(uint8_t i = 0; i < size; i++){
		*(uint32_t*)(RTC_BASE + (uint32_t)(0x50 + i)) = val[i];
	}

	RTC->WPR = 0xFF;							//Enable the write protection for RTC registers
	PWR->CR1 &= ~PWR_CR1_DBP;                   //Запреть доступ к Backup области
}

void write_single_bkp_reg(uint8_t reg_num, uint32_t val){
    if(!(RCC->APB1ENR1 & RCC_APB1ENR1_PWREN))
		RCC->APB1ENR1 |= RCC_APB1ENR1_PWREN;
	PWR->CR1 |= PWR_CR1_DBP;                     //Разрешить доступ к Backup области
	while((RTC->ISR & RTC_ISR_RSF) == 0);        //Wait for RTC APB registers synchronisation
	RTC->WPR = 0xCA;                             //Unlock write protection
	RTC->WPR = 0x53;                             //Unlock write protection
    *(uint32_t *)(RTC_BASE + (0x50 + reg_num * 4)) = val;

	RTC->WPR = 0xFF;							//Enable the write protection for RTC registers
	PWR->CR1 &= ~PWR_CR1_DBP;                   //Запреть доступ к Backup области
}

void RTC_time_difference(RTC_struct_brief *older_time, RTC_struct_brief *newer_time, RTC_struct_brief *difference){
    difference->years = newer_time->years - older_time->years;
    difference->months = newer_time->months - older_time->months;
    difference->date = newer_time->date - older_time->date;
    difference->hours = newer_time->hours - older_time->hours;
    difference->minutes = newer_time->minutes - older_time->minutes;
    difference->seconds = newer_time->seconds - older_time->seconds;
}

const uint16_t day_offset[12] = {0, 31, 61,92, 122, 153, 184, 214, 245, 275,306, 337};
uint32_t RTC_EncodeDateTime(RTC_struct_brief *dt)
{
        uint8_t a = dt->months < 3; // а = 1, если месяц январь или февраль
        int8_t y = dt->years - a;  // y = отнимаем от года 1, если а = 1, а так же 2000;
        uint8_t m = dt->months + 12 * a - 3; // аналогия выражения (12 + month - 3) % 12; делаем март нулевым месяцем года.
        return (dt->date - 1 + day_offset[m] + y * 365 + y / 4 - y / 100 + y / 400) * 86400 +
               (int)dt->hours * 3600 + (int)dt->minutes * 60 + dt->seconds;
}

void RTC_get_time(RTC_struct_brief *br_data)
{
	// we need to clear less bits: (RTC->DR & RTC_DR_DT)
	// and to shift right the part, which we want to --> to normal decimal
    uint32_t timeout = 10000;
	while (!(RTC->ISR & RTC_ISR_RSF) && timeout--);	//  Calendar shadow registers synchronized

	uint32_t TR_buf = 0, DR_buf = 0;

	TR_buf = (RTC->TR);

	br_data->hours = ((((TR_buf & RTC_TR_HT) >> RTC_TR_HT_Pos) * 10) + ((TR_buf & RTC_TR_HU) >> RTC_TR_HU_Pos));
	br_data->minutes = ((((TR_buf & RTC_TR_MNT) >> RTC_TR_MNT_Pos) * 10) + ((TR_buf & RTC_TR_MNU) >> RTC_TR_MNU_Pos));
	br_data->seconds = ((((TR_buf & RTC_TR_ST) >> RTC_TR_ST_Pos) * 10) + ((TR_buf & RTC_TR_SU) >> RTC_TR_SU_Pos));
    br_data->sub_seconds = RTC->TSSSR;

	DR_buf = (RTC->DR);

	br_data->years = ((((DR_buf & RTC_DR_YT) >> RTC_DR_YT_Pos) * 10) + ((DR_buf & RTC_DR_YU) >> RTC_DR_YU_Pos));
	br_data->months = ((((DR_buf & RTC_DR_MT) >> RTC_DR_MT_Pos) * 10) + ((DR_buf & RTC_DR_MU) >> RTC_DR_MU_Pos));
	br_data->date = ((((DR_buf & RTC_DR_DT) >> RTC_DR_DT_Pos) * 10) + ((DR_buf & RTC_DR_DU) >> RTC_DR_DU_Pos));
	br_data->week_day = ((DR_buf & RTC_DR_WDU) >> RTC_DR_WDU_Pos);
}

void RTC_get_alarm(RTC_struct_brief *br_data)
{
	br_data->date = ((((RTC->ALRMAR & RTC_ALRMAR_DT) >> RTC_ALRMAR_DT_Pos) * 10) + ((RTC->ALRMAR & RTC_ALRMAR_DU) >> RTC_ALRMAR_DU_Pos));
	br_data->hours = ((((RTC->ALRMAR & RTC_ALRMAR_HT) >> RTC_ALRMAR_HT_Pos) * 10) + ((RTC->ALRMAR & RTC_ALRMAR_HU) >> RTC_ALRMAR_HU_Pos));
	br_data->minutes = ((((RTC->ALRMAR & RTC_ALRMAR_MNT) >> RTC_ALRMAR_MNT_Pos) * 10) + ((RTC->ALRMAR & RTC_ALRMAR_MNU) >> RTC_ALRMAR_MNU_Pos));
}

void RTC_alarm_init(void)
{
	// unlock write protection
	PWR->CR1 |= PWR_CR1_DBP;
	while((RTC->ISR & RTC_ISR_RSF) == 0);
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;

	// disable Alarm A
	RTC->CR &= ~RTC_CR_ALRAE;
	RTC->ISR &= ~RTC_ISR_ALRAF;

	// wait for Alarm A write flag, to make sure the access to alarm reg is allowed
	while (!(RTC->ISR & RTC_ISR_ALRAWF));
	// date, hours, minutes, seconds mask - Alarm A set if they match
	RTC->ALRMAR |= RTC_ALRMAR_MSK4;		// 0: Alarm A set if the date/day match
	RTC->ALRMAR |= RTC_ALRMAR_MSK3;	// 0: Alarm A set if the hours match
	RTC->ALRMAR |= RTC_ALRMAR_MSK2;		// 0: Alarm A set if the minutes match
	// RTC->ALRMAR &= ~RTC_ALRMAR_MSK4;	// 0: Alarm A set if the date/day match
	// RTC->ALRMAR &= ~RTC_ALRMAR_MSK3;	// 0: Alarm A set if the hours match
	// RTC->ALRMAR &= ~RTC_ALRMAR_MSK2;	// 0: Alarm A set if the minutes match
	RTC->ALRMAR &= ~RTC_ALRMAR_MSK1;		// 1: Seconds don’t care in Alarm A comparison
	// RTC->ALRMAR |= RTC_ALRMAR_MSK1;		// 1: Seconds don’t care in Alarm A comparison

	RTC->ALRMAR &= ~RTC_ALRMAR_WDSEL;	// DU[3:0] field represents the date units
	RTC->ALRMAR &= ~RTC_ALRMAR_PM;		// 0: AM or 24-hour format

	// enable the RTC Alarm interrupt - enable the EXTI Line 18
	// SYSCFG (System configuration controller) clock through APB2 bus enable
	if (!(RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN)) RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	EXTI->IMR1 |= EXTI_IMR1_IM18;	// interrupt request mask - IM18 is not masked now
	EXTI->RTSR1 |= EXTI_RTSR1_RT18;	// rising edge trigger enabled for EXTI line 18

	NVIC_EnableIRQ(RTC_Alarm_IRQn);	// enable the RTC_Alarm IRQ channel in the NVIC
	NVIC_ClearPendingIRQ(RTC_Alarm_IRQn);
	NVIC_SetPriority(RTC_Alarm_IRQn, 0);	// highest priority

	// 1: Alarm A interrupt enabled
	RTC->CR |= RTC_CR_ALRAIE;
	// lock write protection - writing a wrong key reactivates the write protection
	RTC->WPR = 0xFF;
	PWR->CR1 &= ~PWR_CR1_DBP;

	__enable_irq();	// global interrupts enable
}

void RTC_alarm_update(RTC_struct_full *f_data)
{
	// unlock write protection
	PWR->CR1 |= PWR_CR1_DBP;
	while((RTC->ISR & RTC_ISR_RSF) == 0);
	RTC->WPR = 0xCA;
	RTC->WPR = 0x53;
	// disable Alarm A
	RTC->CR &= ~RTC_CR_ALRAE;

	// wait for Alarm A write flag, to make sure the access to alarm reg is allowed
	while (!(RTC->ISR & RTC_ISR_ALRAWF));

	RTC->ALRMAR |= (f_data->date_tens << RTC_ALRMAR_DT_Pos);	// Bits 29:28 DT[1:0]: Date tens in BCD format
	RTC->ALRMAR |= (f_data->date_units << RTC_ALRMAR_DU_Pos);	// Bits 27:24 DU[3:0]: Date units or day in BCD format.
	RTC->ALRMAR |= (f_data->hour_tens << RTC_ALRMAR_HT_Pos);	// Bits 21:20 HT[1:0]: Hour tens in BCD forma
	RTC->ALRMAR |= (f_data->hour_units << RTC_ALRMAR_HU_Pos);	// Bits 19:16 HU[3:0]: Hour units in BCD format.
	RTC->ALRMAR |= (f_data->minute_tens << RTC_ALRMAR_MNT_Pos);	// Bits 14:12 MNT[2:0]: Minute tens in BCD format.
	RTC->ALRMAR |= (f_data->minute_units << RTC_ALRMAR_MNU_Pos);	// Bits 11:8 MNU[3:0]: Minute units in BCD format.

	// enable Alarm A
	RTC->CR |= RTC_CR_ALRAE;

	// enable the RTC Alarm interrupt - enable the EXTI Line 17
	// SYSCFG (System configuration controller) clock through APB2 bus enable
	if (!(RCC->APB2ENR & RCC_APB2ENR_SYSCFGEN))
	{
		RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	}

	EXTI->IMR1 |= EXTI_IMR1_IM18;	// interrupt request mask - IM17 is not masked now
	EXTI->RTSR1 |= EXTI_RTSR1_RT18;	// rising edge trigger enabled for EXTI line 17

	NVIC_EnableIRQ(RTC_Alarm_IRQn);	// enable the RTC_Alarm IRQ channel in the NVIC
	NVIC_ClearPendingIRQ(RTC_Alarm_IRQn);
	NVIC_SetPriority(RTC_Alarm_IRQn, 0);	// highest priority

	// 1: Alarm A interrupt enabled
	RTC->CR |= RTC_CR_ALRAIE;
	// lock write protection - writing a wrong key reactivates the write protection
	RTC->WPR = 0xFF;
	PWR->CR1 &= ~PWR_CR1_DBP;

	__enable_irq();	// global interrupts enable
}
