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

//#include "main.h"

// --------- System condition -----------//
extern ErrorStatus SYSTEM_init_status;
extern ErrorStatus SYSTEM_I2C_error_flag;
extern uint32_t SYSTEM_I2C_error_counter;


extern FIFO(64) fifo;
extern FIFO(64) long_cmd_fifo;
extern FIFO(64) sensors_data_rx_fifo;
extern uint8_t FIFO_flag;

// --------- ADC -----------//
volatile float battary_voltage;
//===========================//

// --------- TIMERS -----------//
extern uint8_t TIM2_finished; // Global flag that indicates status of timer. 0 - still counting; 1 - finished
extern uint8_t TIM7_finished; // Global flag that indicates status of timer. 0 - still counting; 1 - finished
//===========================//
// RTC_struct rtc;
ADC adc;
DS18B20 sensors[12];
OneWire ow;
uint8_t data_buffer[768];
uint16_t buffer_ptr;
uint8_t gs2_buffer[256];
uint8_t gs2_buf_ptr;
uint8_t gs3_buffer[256];
uint8_t gs3_buf_ptr;
RTC_struct_brief current_rtc;

uint16_t WAKEUP_PERIOD_SEC;

void (*handler)();
uint64_t DS18B20_SERIAL_NUMS[12] = {
    0x28ff6402192f66e4,  // 0
    0x28ff6402e388f111,  // 1
    0x28ff6402ed982ca3,  // 2
    0x28ff6402e70b4644,  // 3
    0x28ff6402f959707f,  // 4
    0x28ff6402f927f644,  // 5
    0x28ff6402ed9ae899,  // 6
    0x28ff6402e7088a78,  // 7
    0x28ff6402192e2c18,  // 8
    0x28ff6402ed94bb7c,  // 9
    0x28ff6402e735db59,  // 10
    0x28ff6402ed97c490   // 11
};

void init_global_variables();

#endif /* INC_GLOBAL_VARIABLES_H_ */
