#include "FreeRTOS.h"
#include "buzzer.h"
#include "ds18b20_bus.h"
#include "ff.h"
#include "json.h"
#include "main.h"
#include "microrl.h"
#include "spi.h"
#include "stm32_misc.h"
#include "string.h"
// #include "sx126x.h"
#include "sx127x.h"
#include "system_config.h"
#include "task.h"
#include "xprintf.h"
#include "xmodem.h"
#include "flash.h"
#include "gsm.h"
#include "queue.h"
#include "adc.h"
#include "periph_handlers.h"

#define WATCHDOG_PERIOD_MS 5000

void _close_r(void) {}
void _lseek_r(void) {}
void _read_r(void) {}
void _isatty_r(void) {}
void _fstat_r(void) {}
void _getpid_r(void) {}
void _kill_r(void) {}
void _link_r(void) {}
void _open_r(void) {}
void _times_r(void) {}
void _unlink_r(void) {}

// void _write_r(void) {}

tBuzzer buzzer;
microrl_t rl;
static uint8_t gsm_stream_storage[65];
static uint8_t cli_stream_storage[257];
StaticStreamBuffer_t  gsm_static_stream;
StaticStreamBuffer_t  cli_static_stream;
StaticSemaphore_t xSemaphoreBuffer;
// logging_init_t logger;
LoRa sx127x;
// SX126x SX1268;
XModem xmodem = {
    .delay = vTaskDelay,
    .on_first_packet = FLASH_erase_neighbor,
};
// FRESULT res;
DS18B20 sensors[TEMP_SENSOR_AMOUNT];
DS18B20_BUS sensors_bus;
OneWire ow;
SystemConfig system_config;
char config_json[JSON_STR_CONFIG_SIZE] = {0};
char file_buff[FILE_BUFFER] = {0};

void on_greetings() {
    if (system_config.enable_beep) {
        BUZZ_mario(&buzzer);
    }
    xprintf("Heat up first temperature sensor\n");
}

void on_cooling_down(float deviation) {
    if (system_config.enable_beep) {
        BUZZ_down(&buzzer, 2000, 1000, 100, 30, 2);
    }
    xprintf("Coolling down... Deviation: %.2f\n", deviation);
}

void on_registration_finished() {
    if (system_config.enable_beep) {
        BUZZ_tmnt(&buzzer);
    }
    xprintf("Calibration finished\n");
}

void on_single_registered(uint8_t sensor_num) {
    if (system_config.enable_beep) {
        BUZZ_up(&buzzer, 2000, 3000, 100, 30, 1);
    }
    xprintf("Sensor %d has been registered\n", sensor_num);
}

void notification(int founded_count) {
    if (system_config.enable_beep) {
        BUZZ_beep_repeat(&buzzer, 1500, 100, 500, founded_count);
    }
    xprintf("Last registered sensor num: %d\n", founded_count);
}

void print(const char *buf) {
    xSemaphoreTake(xSemaphore, 1000);
    xprintf(buf);
    xSemaphoreGive(xSemaphore);
}

void sensors_on() { gpio_state(EN_PERIPH, LOW); }

void sensors_off() { gpio_state(EN_PERIPH, HIGH); }

void System_Init() {
    // NVIC_SetPriorityGrouping(0x07);
    xSemaphore = xSemaphoreCreateBinaryStatic( &xSemaphoreBuffer );
    xSemaphoreGive(xSemaphore);
    gsm_stream = xStreamBufferCreateStatic(64, 1, gsm_stream_storage, &gsm_static_stream);
    cli_stream = xStreamBufferCreateStatic(256, 1, cli_stream_storage, &cli_static_stream);
    // vQueueAddToRegistry(gsm_queue, "GSM_QUEUE");
    // vQueueAddToRegistry(cli_queue, "CLI_QUEUE");
    vStreamBufferSetStreamBufferNumber(gsm_stream, 1);
    vStreamBufferSetStreamBufferNumber(cli_stream, 2);

    xdev_out(uart_print);
    system_config_init(&system_config);
    SystemConfigStatus config_status = init_FLASH_system_config(&system_config);
    uint8_t prev_config_err = 0;
    if (config_status == CONFIG_VALIDATION_ERROR) {
        prev_config_err = 1;
        system_config_init(&system_config);
        save_system_config_to_FLASH(&system_config);
    }
    int8_t rtc_status = RTC_Init(system_config.rtc_ppm);
    if (system_config.enable_watchdog) {
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
    // gpio_init(LoRa_DIO1, Input_mode, Open_drain, no_pull, Input);
    gpio_init(LoRa_BUSY, Input_mode, Open_drain, no_pull, Input);
    gpio_init(BUZZ, PB15_TIM15_CH2, Push_pull, no_pull, Very_high_speed);
    gpio_init(EN_PERIPH, General_output, Push_pull, no_pull, Low_speed);
    gpio_init(UART3_TX, PB10_USART3_TX, Open_drain, no_pull, High_speed);

    gpio_init(GSM_TX, PC1_LPUART1_TX, Push_pull, pull_up, High_speed);
    gpio_init(GSM_RX, PC0_LPUART1_RX, Open_drain, no_pull, Input);
    gpio_init(GSM_PWR, General_output, Push_pull, pull_up, Low_speed);

    gpio_state(LED, HIGH);
    gpio_state(GSM_PWR, HIGH);
    gpio_state(EN_SD, LOW);
    gpio_state(EN_LORA, LOW);
    gpio_state(LoRa_NSS, HIGH);
    gpio_state(EN_PERIPH, LOW);

    gpio_exti_init(LoRa_DIO0, 0);
    // gpio_exti_init(LoRa_DIO1, 0);

    UART_init(USART1, 76800, FULL_DUPLEX);


    microrl_init(&rl, print);
    microrl_set_execute_callback(&rl, execute);
    microrl_set_complete_callback(&rl, complet);
    microrl_set_sigint_callback(&rl, sigint);

    RTC_get_time(&current_rtc);

    if ((RCC->CSR & RCC_CSR_IWDGRSTF) || (RCC->CSR & RCC_CSR_WWDGRSTF)) {
        RCC->CSR |= RCC_CSR_RMVF;
        xprintf("\n\033[31mWATCHDOG\033[0m\n");
    }
    if (rtc_status < 0) {
        xprintf("\n\033[31mRTC ERROR\033[0m\n");
        BUZZ_beep_repeat(&buzzer, 300, 200, 300, 3);
    }
    if (prev_config_err) {
        xprintf("\n\033[31mCONFIG ERROR\033[0m\n");
    }
    xprintf("\nSTARTED FROM 0x%08lX ADDRESS\n", SCB->VTOR);
    char rtc_data[25] = {0};
    RTC_string_datetime(rtc_data);
    xprintf("%s\n", rtc_data);
    if (config_status == CONFIG_SAVE_ERROR) {
        xprintf("Saving system config to FLASH failed!\n");
    } else if (config_status == CONFIG_OK) {
        xprintf("System config loaded from FLASH\n");
    } else {
        xprintf("Incorrect init_result! %d\n", config_status);
    }
    write_single_bkp_reg(BCKP_RESET_AMOUNT, RTC->BKP0R + 1);
    xprintf("Amount of resets: %d\n", RTC->BKP0R);
    if (system_config.enable_watchdog) {
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
        .new_rx_data_flag = 0,
        .delay = DWT_Delay_ms,
    };
    if (LoRa_init(&sx127x) == LORA_OK) {
        xprintf("Radio inited\n");
        LoRa_gotoMode(&sx127x, RXCONTIN_MODE);
        sx127x.delay = vTaskDelay;
    } else {
        xprintf("Radio initialization failed!\n");
    }
    // SX1268 = (SX126x){
    //     .gpio = (SX126x_GPIO){
    //         .busy_pin = LoRa_BUSY,
    //         .CS_pin = LoRa_NSS,
    //         .MISO_pin = LoRa_MISO,
    //         .MOSI_pin = LoRa_MOSI,
    //         .SCK_pin = LoRa_SCK,
    //         .reset_pin = EN_LORA,
    //         .DIO1_pin = LoRa_DIO1,
    //         .__MISO_AF_pin = PB4_SPI1_MISO,
    //         .__MOSI_AF_pin = PB5_SPI1_MOSI,
    //         .__SCK_AF_pin = PB3_SPI1_SCK,
    //     },
    //     .spi = LoRa_SPI,
    //     .config = (SX126x_Config){
    //         .frequency = 433000000,
    //         .header_type = 0x00,
    //         .preamble_len = 8,
    //         .bandWidth = 0x05, // BW250
    //         .crcRate = 0x01,  // CR4/5
    //         .crc_on_off = 0x01,
    //         .iq_polarity = 0x00,
    //         .low_data_rate_optim = 0x01,
    //         .packet_type = 0x01, // LoRa
    //         .spreadingFactor = 7,
    //         .power_dbm = 15,
    //         .ramping_time = 0x03,
    //         .sync_word = 0x1424, // 0x12 (0x3444 = 0x34)
    //     },
    //     .rx_data = {0},
    //     .tx_data = {0},
    // };

    buzzer = (tBuzzer){
        .channel = PWM_CH2,
        .TIMx = TIM15,
        .delay = DWT_Delay_ms
    };
    if (system_config.enable_beep) {
        BUZZ_beep(&buzzer, 500, 50);
    }
    // SX126x_Init(&SX1268);
    // uint8_t tx_data[] = "hello world!";
    // SX126x_SendData(&SX1268, tx_data, 12);
    // SX126x_GetStatus(&SX1268);
    // LoRa_transmit(&sx127x, (uint8_t *)"hello world!", 12);
    ow = (OneWire){.uart = USART3};

    for (uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++) {
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
    for (uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++) {
        // sensors[i].serialNumber->serial_code = system_config.sensors_serials[i];
        if (system_config.sensors_serials[i] == 0xFFFFFFFFFFFFFFFF) {
            sensors_bus.is_calibrated = 0;
            sensors_bus.found_amount = 0;
            break;
        }
    }

    // uint8_t devices_count = 0;
    // OW_SearchDevices(&ow, &devices_count);
    // for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
    //     system_config.sensors_serials[i] = ow.ids[i].serial_code;
    //     sensors_bus.sensors[i].serialNumber = &(ow.ids[i]);
    // }
    // xprintf("founded %d devices", devices_count);

    adc = (ADC){
        .ADCx = ADC1,
        .clk_devider = ADC_ClockDevider_1,
        .internal_channels = {.temp = true, .vbat = false, .vref = true},
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
    // ADC_InitRegChannel(&adc, CH9, PA4, SMP_92);
    // ADC_InitRegChannel(&adc, CH10, PA5, SMP_92);
    // ADC_InitRegChannel(&adc, CH12, PA7, SMP_92);
    // ADC_InitRegChannel(&adc, CH13, PC4, SMP_92);
    // ADC_InitRegChannel(&adc, CH15, PB0, SMP_92);
    // ADC_InitRegChannel(&adc, CH14, PC5, SMP_92);
    // ADC_InitRegChannel(&adc, CH6, PA1, SMP_92);
    // ADC_InitRegChannel(&adc, CH1, PC0, SMP_92);
    // ADC_InitRegChannel(&adc, CH2, PC1, SMP_92);
    // ADC_InitRegChannel(&adc, CH3, PC2, SMP_92);
    // ADC_InitRegChannel(&adc, CH4, PC3, SMP_92);
    // ADC_InitRegChannel(&adc, CH5, PA0, SMP_92);
    ADC_InitRegChannel(&adc, VREF, uninitialized, SMP_92);
    // ADC_InitRegChannel(&adc, VBAT, uninitialized, SMP_92);
    ADC_InitRegChannel(&adc, TEMP, uninitialized, SMP_92);
    ADC_Enable(&adc);
    ADC_Start(&adc);

    sim7000g = (GSM){
        .uart = LPUART1,
        .gpio.pwr = GSM_PWR,
        .delay_ms = vTaskDelay,
    };
    UART_init(LPUART1, 9600, FULL_DUPLEX);
    NVIC_SetPriority(LPUART1_IRQn, 11);
    NVIC_SetPriority(USART1_IRQn, 11);


    SDMMC_INIT();
    FATFS fs;
    FRESULT result;
    SystemConfigStatus status;
    SystemConfig flash_config;

    if (SD_Init() == SDR_Success) {
        xprintf("SD Card initialization completed\n");
        result = f_mount(&fs, "", 1);
        if (result == FR_OK) {
            xprintf("SD Card mounted\n");

            memcpy(flash_config.FLASH_page_buffer, system_config.FLASH_page_buffer, CONFIG_SIZE_64 * sizeof(uint64_t));

            status = read_config_from_SD(&system_config, SYSTEM_CONFIG_PATH, config_json, JSON_STR_CONFIG_SIZE);
            if (status == CONFIG_SD_EMPTY) {
                status = save_config_to_SD(&system_config, SYSTEM_CONFIG_PATH, config_json);
                if (status != CONFIG_OK) {
                    xprintf("Cant save system config to SD card\n");
                } else {
                    xprintf("System config saved to SD card\n");
                }
            } else {
                if (memcmp(system_config.FLASH_page_buffer, flash_config.FLASH_page_buffer,
                           CONFIG_SIZE_64 * sizeof(uint64_t))) {
                    xprintf("System config loaded from SD card\n");
                    save_system_config_to_FLASH(&system_config);
                    if (system_config.immediate_applying) {
                        xprintf(
                            "Immediately applying option has been activated."
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
    int16_t temp = ADC_internal_temp(adc.reg_channel_queue[1].result, adc.vdda_mvolt);
    xprintf("Vref: %d mv\n", adc.vdda_mvolt);
    xprintf("Temp: %d C\n", temp);

    rl.print(rl.prompt_str);
    buzzer.delay = vTaskDelay;
    sx127x.delay = vTaskDelay;
}
