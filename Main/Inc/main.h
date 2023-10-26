#ifndef INC_MAIN_H_
#define INC_MAIN_H_
#include "stm32l4xx.h"
#include <stdio.h>
// #include "formating_output.h"
#include "delay.h"
#include "gpio.h"
#include "rcc.h"
#include "pwm.h"
#include "uart.h"
#include "spi.h"
#include "dwt.h"
// #include "i2c.h"
// #include "tmp1075.h"
#include "adc.h"
#include "global_variables.h"
#include "periph_handlers.h"
#include "tim.h"
#include "fifo.h"
// #include "sx1278.h"
#include "rtc.h"
#include "sdio.h"
#include "one_wire.h"
#include "ds18b20.h"
// #include "sx126x.h"
// #include "gsm.h"
// #include "cli.h"
#include "flash.h"
#include "fat32.h"
#include "buzzer.h"
#include "low_power.h"
#include "iwdg.h"

// #define SCREEN_PLUGGED
// #define SENSORS_PLUGGED

#define V3_0

#ifdef V3_1
#define LED 		PC13

#define EN_PERIPH   PB8

#define BUZZ        PA3
#define UART2_TX    PA2

#define UART3_RX	PB11
#define UART3_TX	PB10

#define I2C1_SDA	PB9
#define I2C1_SCL	PB8


#define TEMP_SENSOR_PWR_EN    PC7
#define SOIL_SENSOR_PWR_EN    PA8

#define LoRa_SPI          SPI2
#define LoRa_BUSY         PB12
#define LoRa_SCK          PB13
#define LoRa_MISO         PB14
#define LoRa_MOSI         PB15
#define LoRa_NSS          PC6
#define LoRa_DIO1         PB6
#define LoRa_DIO2         PB5
#define LoRa_NRESET       PA15
#endif

#ifdef V3_0
#define LED 		PC13

#define EN_PERIPH   PB4

#define BUZZ        PA3
#define UART2_TX    PA2

#define UART3_RX	PB11
#define UART3_TX	PB10

#define I2C1_SDA	PB9
#define I2C1_SCL	PB8


#define TEMP_SENSOR_PWR_EN    PC7
#define SOIL_SENSOR_PWR_EN    PA8

#define LoRa_SPI          SPI2
#define LoRa_BUSY         PB12
#define LoRa_SCK          PB13
#define LoRa_MISO         PB14
#define LoRa_MOSI         PB15
#define LoRa_NSS          PC6
#define LoRa_DIO1         PC7
#define LoRa_NRESET       PB2

#define GSM_RX          PB11
#define GSM_TX          PB10
#define GSM_CTS         PA6
#define GSM_RTS         PB1
#define GSM_DCD         PA10
#define GSM_RI          PA11
#define GSM_DTR         PA12
#define GSM_PWR         PC13
#define GSM_NRESET      PB7
#endif

#define SD_D0				PC9
#define SD_D1				PC8
#define SD_D2				PC10
#define SD_D3				PC11
#define SD_CK				PC12
#define SD_CMD				PD2


#define SELF_ID        1
//-----------------------//

#endif /* INC_MAIN_H_ */
