#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "string.h"
#include "sx126x.h"
#include "microrl.h"
#include "stm32_misc.h"
#include "buzzer.h"
#include "sx127x.h"
#include "xprintf.h"
#include "spi.h"
#include "ff.h"
#include "ds18b20_bus.h"


#define BUFFER_SIZE             256
#define TEMP_SENSOR_AMOUNT      12
#define MOISTURE_SENSOR_AMOUNT  11
#define FLASH_CONFIG_OFFSET     30
#define FLASH_PAGE              63
#define WATCHDOG_PERIOD_MS      5000
#define WAKEUP_PERIOD_SEC       60*20

void _close_r(void) {}
void _lseek_r(void) {}
void _read_r(void) {}
void _isatty_r(void) {}
void _fstat_r(void) {}
void _getpid_r(void) {}
void _kill_r(void) {}

// void _write_r(void) {}


tBuzzer buzzer;
microrl_t rl;
logging_init_t logger;
LoRa sx127x;
FILINFO Finfo;
uint64_t sorted_serials[12];
DS18B20 sensors[12];
DS18B20_BUS sensors_bus;
OneWire ow;
uint64_t DS18B20_SERIAL_NUMS[12];

void my_print(int data){
    UART_tx(USART1, (uint8_t)(data));
}

void on_greetings(){
    BUZZ_beep(&buzzer, 400, 30);
    BUZZ_beep(&buzzer, 800, 30);
}

void on_registration_finished(){
    BUZZ_tmnt(&buzzer);
}

void System_Init(){
    xfunc_output = my_print;
    // uint64_t sorted_serials[12] = {0};
    // setvbuf(stdout, NULL, _IONBF, 0);
    // SysTick_Config(millisec);
    // RCC_init_MSI();
    uint8_t is_first_init = RTC_Init();
    // RTC_auto_wakeup_enable(WAKEUP_PERIOD_SEC);

    // IWDG_init(WATCHDOG_PERIOD_MS);
    // IWDG_disable_in_debug();
    DWT_Init();

    gpio_init(LED, General_output, Push_pull, no_pull, Low_speed);
    gpio_init(EN_SD, General_output, Push_pull, no_pull, Low_speed);
    gpio_init(EN_LORA, General_output, Push_pull, no_pull, Low_speed);
    gpio_init(UART1_TX, PB6_USART1_TX, Push_pull, pull_up, High_speed);
    gpio_init(UART1_RX, PB7_USART1_RX, Open_drain, no_pull, Input);
    gpio_init(LoRa_MOSI, PB5_SPI1_MOSI, Push_pull, no_pull, High_speed);
    gpio_init(LoRa_MISO, PB4_SPI1_MISO, Open_drain, no_pull, Input);
    gpio_init(LoRa_SCK, PB3_SPI1_SCK, Push_pull, no_pull, High_speed);
    gpio_init(LoRa_NSS, General_output, Push_pull, no_pull, High_speed);
    gpio_init(LoRa_DIO0, Input_mode, Open_drain, no_pull, Input);
    gpio_init(BUZZ, PB15_TIM15_CH2, Push_pull, no_pull, Very_high_speed);
    gpio_init(EN_PERIPH, General_output, Push_pull, no_pull, Low_speed);
    gpio_init(UART3_TX, PB10_USART3_TX, Push_pull, pull_up, High_speed);

    gpio_state(EN_SD, LOW);
    gpio_state(EN_PERIPH, LOW);
    gpio_state(EN_LORA, LOW);
    gpio_state(LoRa_NSS, HIGH);

    gpio_exti_init(LoRa_DIO0, 0);

    UART_init(USART1, 9600, FULL_DUPLEX);

    buzzer = (tBuzzer){
        .channel = PWM_CH2,
        .TIMx = TIM15,
        .delay = DWT_Delay_ms
    };

    microrl_init(&rl, print);
    microrl_set_execute_callback (&rl, execute);
	microrl_set_complete_callback (&rl, complet);
    microrl_set_sigint_callback (&rl, sigint);

    RTC_get_time(&current_rtc);
    logger = (logging_init_t){
        .write_function=print,
        .get_time_string=RTC_string_datetime,
        .default_level=LOGGING_LEVEL_DEBUG
    };
    logging_init(&logger);
    uint8_t val = 2;
    if(is_first_init){
        xprintf("First initialization. You need to set MCU time.", val);
    }
    xprintf("STARTED FROM 0x%X ADDRESS\r", SCB->VTOR);
    // LOG_DEBUG("debug message %d\r", val);
    // LOG_INFO("debug message %.2f\r", 2.423);
    // LOG_INFO("info message %d\r", val);
    // LOG_WARN("warning message %d\r", val);
    // LOG_ERROR("error message %d\r", val);

    if((RCC->CSR & RCC_CSR_IWDGRSTF) || (RCC->CSR & RCC_CSR_WWDGRSTF)){
        RCC->CSR |= RCC_CSR_RMVF;
        LOG_ERROR("WATCHDOG \r");
    }

    spi_init(LoRa_SPI, div_4, Mode_0, data_8_bit, MSB);

    sx127x = (LoRa){
        .LoRaSPI = LoRa_SPI,
        .reset_pin = EN_LORA,
        .CS_pin = LoRa_NSS,
        .DIO0_pin = LoRa_DIO0,
        .bandWidth = BW_125KHz,
        .freq_mhz = 435,
        .power = POWER_11db,
        .preamble = 8,
        .codingRate = CR_4_5,
        .overCurrentProtection = 120,
        .spredingFactor = SF_10,
        .ldro = 1,
        .got_new_packet = 0,
        .delay = DWT_Delay_ms,
    };
    LoRa_init(&sx127x);
    LoRa_gotoMode(&sx127x, RXCONTIN_MODE);
    sx127x.delay = vTaskDelay;

    SDMMC_INIT();
    SDResult result = SD_Init();
    FATFS fs;
    FIL file;
    DIR dir;
    FRESULT res;
    FILINFO fno;


    if(result == SDR_Success){
        LOG_INFO("SD Card initialization completed");
        if (f_mount(&fs, "", 1) == FR_OK) {
            LOG_INFO("SD Card mounted");
            res = f_opendir(&dir, "");
            if (res == FR_OK){
                res = f_readdir(&dir, &fno);
            }
            if (f_open(&file, "1stfile.txt", FA_OPEN_ALWAYS | FA_READ | FA_WRITE) == FR_OK) {
                UINT written_count = 0;
                char data[] = "First string in my file\n";
                f_lseek(&file, file.obj.objsize);
                if (f_write(&file, data, strlen(data), &written_count) > 0) {

                }
                f_close(&file);
            }

            //Unmount drive, don't forget this!
            // f_mount(0, "", 1);
        } else {
            LOG_ERROR("SD Card not mounted");
        }
    } else {
        LOG_ERROR("SD Card initialization failed");
    }

    // buzzer.mario(&buzzer);
    ow = (OneWire){.uart=USART3};

    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        sensors[i] = (DS18B20){0};
        sensors[i].ow = &ow;
    }
    sensors_bus = (DS18B20_BUS){
        .amount = 12,
        .delay_ms = vTaskDelay,
        .ow = &ow,
        .sensors = sensors,
        .serials = sorted_serials,
        .greetings = on_greetings,
        .on_registration_finished = on_registration_finished,
    };
    uint8_t dev_num = OneWire_SearchDevices(sensors_bus.ow);
    xprintf("devices: %d\n\r", dev_num);
    for(uint8_t i = 0; i < sensors_bus.amount; i++){
        // initial_serials[i] = sensors_bus.ow->ids[i].serial_code;
        sensors_bus.sensors[i].serialNumber = &(sensors_bus.ow->ids[i]);
    }
    TemperatureSensorsMeasure(&sensors_bus, 0);

//     gpio_state(EN_PERIPH, HIGH);

    // FLASH_read(FLASH_PAGE, FLASH_CONFIG_OFFSET, DS18B20_SERIAL_NUMS, TEMP_SENSOR_AMOUNT);
    // if(DS18B20_SERIAL_NUMS[0] == 0xFFFFFFFFFFFFFFFF){
    //     if(Calibration_routine(sensors, &buzzer, sorted_serials)){
    //         FLASH_write(FLASH_PAGE, FLASH_CONFIG_OFFSET, sorted_serials, TEMP_SENSOR_AMOUNT);
    //         FLASH_read(FLASH_PAGE, FLASH_CONFIG_OFFSET, DS18B20_SERIAL_NUMS, TEMP_SENSOR_AMOUNT);
    //     }
    // }
    buzzer.delay = vTaskDelay;
    adc = (ADC){
        .ADCx = ADC1,
        .clk_devider = ADC_ClockDevider_1,
        .internal_channels = {
            .temp = false,
            .vbat = false,
            .vref = true
        },
        .resolution = ADC_12bit,
        .mode = ADC_SINGLE_MODE,
        .trigger.polarity = ADC_Software_trigger,
        .ovrsmpl_ratio = OVRSMPL_32x,
        .delay_ms = DWT_Delay_ms,
    };
    ADC_Init(&adc);
    /*
                    6 (50cm)        5 (40cm)        4 (30cm)        3 (20cm)        2 (10cm)        1 (0cm)     VCC  GND
                    (CH14, PC5)     (CH15, PB0)     (CH13, PC4)     (CH12, PA7)     (CH10, PA5)     (CH9, PA4)
    VCC GNS TMP_S
                    NC              11 (100cm)      10 (90cm)       9 (80cm)        8 (75cm)        7 (60cm)    VCC  GND
                    (CH6, PA1)      (CH1, PC0)      (CH2, PC1)      (CH3, PC2)      (CH4, PC3)      (CH5, PA0)
    */
    ADC_InitRegChannel(&adc, CH9, PA4, SMP_92);
    ADC_InitRegChannel(&adc, CH10, PA5, SMP_92);
    ADC_InitRegChannel(&adc, CH12, PA7, SMP_92);
    ADC_InitRegChannel(&adc, CH13, PC4, SMP_92);
    ADC_InitRegChannel(&adc, CH15, PB0, SMP_92);
    ADC_InitRegChannel(&adc, CH14, PC5, SMP_92);
    // ADC_InitRegChannel(&adc, CH6, PA1, SMP_92);
    ADC_InitRegChannel(&adc, CH1, PC0, SMP_92);
    ADC_InitRegChannel(&adc, CH2, PC1, SMP_92);
    ADC_InitRegChannel(&adc, CH3, PC2, SMP_92);
    ADC_InitRegChannel(&adc, CH4, PC3, SMP_92);
    ADC_InitRegChannel(&adc, CH5, PA0, SMP_92);
    ADC_InitRegChannel(&adc, VREF, uninitialized, SMP_92);
//     // ADC_InitRegChannel(&adc, VBAT, uninitialized, SMP_92);
//     // ADC_InitRegChannel(&adc, TEMP, uninitialized, SMP_92);
    ADC_Enable(&adc);
    ADC_Start(&adc);
//     TemperatureSensorsMeasure(sensors, TEMP_SENSOR_AMOUNT, 1);
//     ADC_WaitMeasures(&adc, 1000000);
//     gpio_state(EN_PERIPH, LOW);
    // RTC_get_time(&current_rtc);

//     uint32_t vdda = adc.vdda_mvolt;
//     // float internal_temp = ADC_internal_temp(adc.reg_channel_queue[13].result);

//     char str[BUFFER_SIZE] = {0};


//     gpio_state(EN_SD, HIGH);
//     Delay(20);
//     RCC->CRRCR |= RCC_CRRCR_HSI48ON;
//     while(!(RCC->CRRCR & RCC_CRRCR_HSI48RDY));
    // SDMMC_INIT();
    // SDResult result = SD_Init();
//     FAT32t fat32;
//     FAT32_File file;
//     uint32_t wrote_count = 0;
//     char ser_num[20];
//     char initail_string[] = "\nTimestamp\tT1(-5 cm)\tT2(0 cm)\tT3(10 cm)\tT4(20 cm)\tT5(30 cm)\tT6(40 cm)\tT7(50 cm)\t
// T8(60 cm)\tT9(70 cm)\tT10(80 cm)\tT11(90 cm)\tT12(100 cm)\tA1(0 cm)\tA2(10 cm)\tA3(20 cm)\tA4(30 cm)\tA5(40 cm)\t
// A6(50 cm)\tA7(60 cm)\tA8(70 cm)\tA9(80 cm)\tA10(90 cm)\tA11(100 cm)\tVref_mV\t\t
// (T1-T12 [degree]; A1-A11 [ADC quantum])\n";
//     fat32 = FAT32();
//     if(fat32.last_status == OK){
//         file = fat32.open(&fat32, "text1.txt");
//         if(file.file_size == 0){
//             wrote_count += file.append(&file, "Temperature sensors serials:\n", 29);
//             for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
//                 snprintf(ser_num, 12, "%#08x\n", DS18B20_SERIAL_NUMS[i]);
//                 wrote_count += file.append(&file, ser_num, strlen(ser_num));
//             }
//             wrote_count += file.append(&file, initail_string, strlen(initail_string));
//         }
//         if(file.status == OK){
//             uint16_t counter = RTC_string_datetime(&current_rtc, str);
//             counter += DS18B20_array_to_str(sensors, TEMP_SENSOR_AMOUNT, str, BUFFER_SIZE, counter);
//             counter += ADC_array_to_str(&adc, MOISTURE_SENSOR_AMOUNT, str, BUFFER_SIZE, counter);
//             wrote_count += file.append(&file, str, strlen(str));
//         }
//         else {
//             buzzer.down(&buzzer, 900, 500, 30, 30, 3);
//             buzzer.beep_repeat(&buzzer, 800, 100, 200, 3);
//         }
//     }
//     else {
//         buzzer.down(&buzzer, 1500, 1100, 30, 10, 3);
//     }
//     // buzzer.beep_repeat(&buzzer, 1500, 100, 100, 2);
//     gpio_state(EN_SD, LOW);
//     RCC->CRRCR &= ~RCC_CRRCR_HSI48ON;
//     buzzer.beep(&buzzer, 400, 30);
//     buzzer.beep(&buzzer, 800, 30);
//     // Delay(3000);
//     stop_cortex();

}


