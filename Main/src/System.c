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
#include "system_config.h"
#include "json.h"


#define WATCHDOG_PERIOD_MS      2000
#define WAKEUP_PERIOD_SEC       60*20
#define JSON_CONFIG_SIZE        512

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
// logging_init_t logger;
LoRa sx127x;

// FRESULT res;
DS18B20 sensors[12];
DS18B20_BUS sensors_bus;
OneWire ow;
SystemConfig system_config;
char config_json[JSON_CONFIG_SIZE] = {0};


void on_greetings(){
    if(system_config.enable_beep){
        BUZZ_mario(&buzzer);
    }
    xprintf("Heat up first temperature sensor\n");
}

void on_cooling_down(float deviation){
    if(system_config.enable_beep){
        BUZZ_down(&buzzer, 2000, 1000, 100, 30, 2);
    }
    xprintf("Coolling down... Deviation: %.2f\n", deviation);
}

void on_registration_finished(){
    if(system_config.enable_beep){
        BUZZ_tmnt(&buzzer);
    }
    xprintf("Calibration finished\n");
}

void on_single_registered(uint8_t sensor_num){
    if(system_config.enable_beep){
        BUZZ_up(&buzzer, 2000, 3000, 100, 30, 1);
    }
    xprintf("Sensor %d has been registered\n", sensor_num);
}

void notification(int founded_count){
    if(system_config.enable_beep){
        BUZZ_beep_repeat(&buzzer, 1500, 100, 500, founded_count);
    }
    xprintf("Last registered sensor num: %d\n", founded_count);
}

void print(const char *buf){
    xprintf(buf);
}

void sensors_on(){
    gpio_state(EN_PERIPH, LOW);
}

void sensors_off(){
    gpio_state(EN_PERIPH, HIGH);
}

void System_Init(){
    xdev_out(uart_print);
    system_config_init(&system_config);
    SystemConfigStatus config_status = init_FLASH_system_config(&system_config);
    uint8_t prev_config_err = 0;
    if(config_status == CONFIG_VALIDATION_ERROR){
        prev_config_err = 1;
        system_config_init(&system_config);
        save_system_config_to_FLASH(&system_config);
    }
    // uint64_t sorted_serials[12] = {0};
    // setvbuf(stdout, NULL, _IONBF, 0);
    // SysTick_Config(millisec);
    // RCC_init_MSI();
    // uint8_t is_first_init =
    int8_t rtc_status = RTC_Init(system_config.rtc_ppm);
    // if(is_first_init){
    //     RTC_set_date(19 << 20 | 80 << 16);  // just for set ISR_INITS flag
    // }
    // RTC_auto_wakeup_enable(WAKEUP_PERIOD_SEC);
    if(system_config.enable_watchdog){
        IWDG_init(WATCHDOG_PERIOD_MS);
        IWDG_disable_in_debug();
    }
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
    gpio_state(EN_LORA, LOW);
    gpio_state(LoRa_NSS, HIGH);

    gpio_exti_init(LoRa_DIO0, 0);

    UART_init(USART1, system_config.uart_speed, FULL_DUPLEX);

    microrl_init(&rl, print);
    microrl_set_execute_callback (&rl, execute);
	microrl_set_complete_callback (&rl, complet);
    microrl_set_sigint_callback (&rl, sigint);

    RTC_get_time(&current_rtc);

    // logger = (logging_init_t){
    //     .write_function=print,
    //     .get_time_string=RTC_string_datetime,
    //     .default_level=LOGGING_LEVEL_DEBUG
    // };
    // logging_init(&logger);
    if((RCC->CSR & RCC_CSR_IWDGRSTF) || (RCC->CSR & RCC_CSR_WWDGRSTF)){
        RCC->CSR |= RCC_CSR_RMVF;
        xprintf("\n\033[31mWATCHDOG\033[0m\n");
    }
    if(rtc_status < 0){
        xprintf("\n\033[31mRTC ERROR\033[0m\n");
        BUZZ_beep_repeat(&buzzer, 300, 200, 300, 3);
    }
    if(prev_config_err){
        xprintf("\n\033[31mCONFIG ERROR\033[0m\n");
    }
    xprintf("\nSTARTED FROM 0x%08lX ADDRESS\n", SCB->VTOR);
    char rtc_data[25] = {0};
    RTC_string_datetime(rtc_data);
    xprintf("%s\n", rtc_data);
    if(config_status == CONFIG_SAVE_ERROR){
        xprintf("Saving system config to FLASH failed!\n");
    } else if(config_status == CONFIG_OK){
        xprintf("System config loaded from FLASH\n");
    } else {
        xprintf("Incorrect init_result! %d\n", config_status);
    }

    if(system_config.enable_watchdog){
        FLASH_disable_iwdg_stby();
    }


    spi_init(LoRa_SPI, div_4, Mode_0, data_8_bit, MSB);

    sx127x = (LoRa){
        .LoRaSPI = LoRa_SPI,
        .reset_pin = EN_LORA,
        .CS_pin = LoRa_NSS,
        .DIO0_pin = LoRa_DIO0,
        .bandWidth = system_config.lora_bw,
        .freq_mhz = system_config.lora_freq,
        .power = system_config.lora_tx_power,
        .preamble = system_config.lora_preamble,
        .codingRate = system_config.lora_cr,
        .spredingFactor = system_config.lora_sf,
        .ldro = system_config.lora_ldro,
        .overCurrentProtection = 120,
        .got_new_packet = 0,
        .delay = DWT_Delay_ms,
    };
    if(LoRa_init(&sx127x) == LORA_OK){
        xprintf("Radio inited\n");
        LoRa_gotoMode(&sx127x, RXCONTIN_MODE);
        sx127x.delay = vTaskDelay;
    } else {
        xprintf("Radio initialization failed!\n");
    }
    buzzer = (tBuzzer){
        .channel = PWM_CH2,
        .TIMx = TIM15,
        .delay = DWT_Delay_ms
    };
    if(system_config.enable_beep){
        BUZZ_beep(&buzzer, 500, 50);
    }
    ow = (OneWire){.uart=USART3};

    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        sensors[i] = (DS18B20){0};
        sensors[i].ow = &ow;
    }
    sensors_bus = (DS18B20_BUS){
        .connected_amount = TEMP_SENSOR_AMOUNT,
        .found_amount = TEMP_SENSOR_AMOUNT,
        .is_calibrated = 1,
        .delay_ms = vTaskDelay,
        .ow = &ow,
        .sensors = sensors,
        .serials = system_config.sensors_serials,
        .greetings = on_greetings,
        .on_registration_finished = on_registration_finished,
        .on_cooling_down = on_cooling_down,
        .on_single_registered = on_single_registered,
        .notification = notification,
        .power_on = sensors_on,
        .power_off = sensors_off,
    };
    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        // sensors[i].serialNumber->serial_code = system_config.sensors_serials[i];
        if(system_config.sensors_serials[i] == 0xFFFFFFFFFFFFFFFF){
            sensors_bus.is_calibrated = 0;
            sensors_bus.found_amount = 0;
            break;
        }
    }

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
    VCC GND TMP_S
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
    // ADC_InitRegChannel(&adc, VBAT, uninitialized, SMP_92);
//     // ADC_InitRegChannel(&adc, TEMP, uninitialized, SMP_92);
    ADC_Enable(&adc);

    SDMMC_INIT();
    UINT written_count = 0;
    FATFS fs;
    FRESULT result;
    FIL _file;
    SystemConfigStatus status;
    OW_Status ow_status;
    SystemConfig flash_config;
    char sd_log[1024] = {0};
    uint16_t sd_ptr = 0;

    if(SD_Init() == SDR_Success){
        xprintf("SD Card initialization completed\n");
        result = f_mount(&fs, "", 1);
        if (result == FR_OK) {
            xprintf("SD Card mounted\n");

            memcpy(flash_config.FLASH_page_buffer,
                   system_config.FLASH_page_buffer,
                   CONFIG_SIZE_64 * sizeof(uint64_t));

            status = read_config_from_SD(&system_config, SYSTEM_CONFIG_PATH,
                                         config_json, JSON_CONFIG_SIZE);
            if(status == CONFIG_SD_EMPTY){
                status = save_config_to_SD(&system_config, SYSTEM_CONFIG_PATH,
                                           config_json);
                if(status != CONFIG_OK){
                    xprintf("Cant save system config to SD card\n");
                } else {
                    xprintf("System config saved to SD card\n");
                }
            } else {
                if(memcmp(system_config.FLASH_page_buffer,
                          flash_config.FLASH_page_buffer,
                          CONFIG_SIZE_64 * sizeof(uint64_t))){
                    xprintf("System config loaded from SD card\n");
                    save_system_config_to_FLASH(&system_config);
                    if(system_config.immediate_applying){
                        xprintf("Immediately applying option has been activated."\
                                "\nFor applying new config the system will be restarted\n");
                        DWT_Delay_ms(1);
                        __NVIC_SystemReset();
                    } else {
                        xprintf("For applying new config restart system\n");
                    }
                }

            }
        } else {
            xprintf("SD Card not mounted\n");
        }
    } else {
        xprintf("SD Card initialization failed\n");
    }


    if(system_config.action_mode == 1){
        RTC_auto_wakeup_enable(system_config.wakeup_period);
        sensors_bus.power_on();
        DWT_Delay_ms(50);
        ADC_Start(&adc);
        if(sensors_bus.found_amount == 0){
            ow_status = OW_EMPTY_BUS;
        } else {
            ow_status = TemperatureSensorsMeasure(&sensors_bus, sensors_bus.is_calibrated);
        }
        ADC_WaitMeasures(&adc, 10000);
        sensors_bus.power_off();

        f_open(&_file, "measures.log", FA_OPEN_ALWAYS | FA_READ  | FA_WRITE);
        if(f_size(&_file) == 0){
            sd_ptr += xsprintf(sd_log + sd_ptr, "Module id: %d\n\n", system_config.module_id);
            for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
                sd_ptr += xsprintf(sd_log + sd_ptr, "T%02d: [0x%016llX]\n", i + 1,
                                    sensors_bus.is_calibrated ?
                                    sensors_bus.serials[i] :
                                    sensors_bus.sensors[i].serialNumber->serial_code);
            }
            sd_ptr += xsprintf(sd_log + sd_ptr, "\n");
            sd_ptr += xsprintf(sd_log + sd_ptr, "%-25s", "Timestamp");
            for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
                sd_ptr += xsprintf(sd_log + sd_ptr, "T%02d    ", i + 1);
            }
            for(uint8_t i = 0; i < 11; i++){
                sd_ptr += xsprintf(sd_log + sd_ptr, "A%02d   ", i + 1);
            }
            sd_ptr += xsprintf(sd_log + sd_ptr, "VDDA\n");
        }

        sd_ptr += RTC_string_datetime(sd_log + sd_ptr);
        sd_ptr += xsprintf(sd_log + sd_ptr, "  ");
        for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
            sd_ptr += xsprintf(sd_log + sd_ptr, "%.2f  ",
                                sensors_bus.sensors[i].temperature);
        }
        for(uint8_t i = 0; i < 11; i++){
            sd_ptr += xsprintf(sd_log + sd_ptr, "%-4d  ",
                                adc.reg_channel_queue[i].result_mv);
        }
        sd_ptr += xsprintf(sd_log + sd_ptr, "%-4d\n", adc.vdda_mvolt);
        f_lseek(&_file, _file.obj.objsize);
        f_write(&_file, sd_log, sd_ptr, &written_count);
        f_close(&_file);

        if(ow_status != OW_OK) {
            f_open(&_file, "errors.log", FA_OPEN_ALWAYS | FA_READ  | FA_WRITE);
            f_lseek(&_file, _file.obj.objsize);
            sd_ptr += RTC_string_datetime(sd_log + sd_ptr);
            sd_ptr += xsprintf(sd_log + sd_ptr, "  ");
            if(ow_status == OW_EMPTY_BUS){
                sd_ptr += xsprintf(sd_log + sd_ptr, "Empty bus\n");
            } else if(ow_status == OW_TIMEOUT){
                sd_ptr += xsprintf(sd_log + sd_ptr, "Timeout\n");
            } else if(ow_status == OW_ROM_FINDING_ERROR){
                sd_ptr += xsprintf(sd_log + sd_ptr, "ROM_FINDING_ERROR\n");
            } else {
                sd_ptr += xsprintf(sd_log + sd_ptr, "ERROR\n");
            }
            f_write(&_file, sd_log, strlen(sd_log), &written_count);
            f_close(&_file);
        }
        f_mount(0, "", 1);
        stop_cortex();
    }

    rl.print(rl.prompt_str);
    buzzer.delay = vTaskDelay;
    sx127x.delay = vTaskDelay;

}


