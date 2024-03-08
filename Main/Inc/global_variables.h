#ifndef INC_GLOBAL_VARIABLES_H_
#define INC_GLOBAL_VARIABLES_H_

#include "stm32l4xx.h"
#include "fifo.h"
// #include "sx1278.h"
#include "rtc.h"
// #include "gsm.h"
#include "one_wire.h"
#include "ds18b20.h"
// #include "sx126x.h"
#include "adc.h"
#include "logging.h"
#include "../../console/inc/microrl.h"
//#include "main.h"

// --------- System condition -----------//
extern ErrorStatus SYSTEM_init_status;
extern ErrorStatus SYSTEM_I2C_error_flag;
extern uint32_t SYSTEM_I2C_error_counter;


// RTC_struct rtc;
extern ADC adc;
extern DS18B20 sensors[12];
extern OneWire ow;
extern RTC_struct_brief current_rtc;
extern logging_init_t logger;
extern volatile unsigned long ulHighFrequencyTimerTicks;
extern microrl_t rl;
extern microrl_t *prl;

extern uint16_t WAKEUP_PERIOD_SEC;

extern void (*handler)();
extern uint64_t DS18B20_SERIAL_NUMS[12];

void init_global_variables();

#endif /* INC_GLOBAL_VARIABLES_H_ */
