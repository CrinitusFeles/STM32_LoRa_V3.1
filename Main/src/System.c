#include "main.h"
#include "string.h"
#include "stdlib.h"


#define BUFFER_SIZE             256
#define TEMP_SENSOR_AMOUNT      12
#define MOISTURE_SENSOR_AMOUNT  11
#define CALIBRATION_TEMP_DELTA  3
#define FLASH_CONFIG_OFFSET     30
#define FLASH_PAGE              63
#define WATCHDOG_PERIOD_MS      3000
#define WAKEUP_PERIOD_SEC       20//60*20

void _close_r(void) {}
void _lseek_r(void) {}
void _read_r(void) {}
void _write_r(void) {}

void TemperatureSensorsMeasure(DS18B20 *sensors, uint8_t sensors_count, uint8_t is_sorted){
    OneWireStatus status;
    DS18B20_StartTempMeas(sensors[0].ow);
    for(uint8_t i = 0; i < sensors_count; i++){
        if(is_sorted){
            status = OneWire_MatchRom(sensors[i].ow, (RomCode*)(&DS18B20_SERIAL_NUMS[i]));
        } else{
            status = OneWire_MatchRom(sensors[i].ow, sensors[i].serialNumber);
        }
        if(status == ONE_WIRE_OK){
            OneWire_Write(sensors[0].ow, DS18B20_READ_SCRATCHPAD);
            OneWire_ReadArray(sensors[0].ow, (uint8_t *)(&(sensors[i].scratchpad)), 9);
            sensors[i].temperature = (uint32_t)(sensors[i].scratchpad.temperature) * 0.0625;
        } else {
            Delay(1);
        }
    }
}

float max_temp_deviation(float *temperatures, uint8_t temp_amount){
    float max_temp = temperatures[0];
    float min_temp = temperatures[0];
    for(uint8_t i = 0; i < temp_amount; i++){
        if(temperatures[i] > max_temp)
            max_temp = temperatures[i];
        if(temperatures[i] < min_temp)
            min_temp = temperatures[i];
    }
    return max_temp - min_temp;
}

uint8_t is_in_array(uint8_t value, uint8_t *array, uint8_t length){
    for(uint8_t i = 0; i < length; i++){
        if(array[i] == value)
            return 1;
    }
    return 0;
}

void change_list_order(uint64_t *initial_array, uint8_t *order_list, uint8_t length){
    uint64_t buffer = 0;
    for(uint8_t i = 0; i < length; i++){
        buffer = initial_array[i];
        initial_array[i] = initial_array[order_list[i]];
        initial_array[order_list[i]] = buffer;
    }
}

uint8_t is_unic_set(uint8_t *array, uint8_t length){
    uint16_t sum = 0;
    for(uint8_t i = 0; i < length; i++){
        sum += array[i];
    }
    uint16_t val = (uint16_t)((length - 1) / 2.0 * length);
    if(sum != val)
        return 0;
    return 1;
}

/*
Процесс распознавания температурных датчиков на трубе. После определения серийных номеров датчиков на шине неизвестно
их расположение в трубе. Для этого необходимо последовательно сверху вниза програвать температурный датчик. Перед
началом процедуры датчики записывают свою начальную температуру. При превыщении порога температуры записывается индекс
датчика и в дальнейшем значение этого датчика не сравнивается с порогом.

В итоге получим массив индексов, которые соответствуют порядку нагревания. Т.е. массив с индексами [5, 1, 0, 3, 2, 4]
говорит о том, что сначала нагревался датчик с индексом 5, потом 1 и т.д. Далее потребуется просто отсортировать массив
датчиков в соответствии с порядком нагревания.
*/
void RecognitionRoutine(DS18B20 *sensors, tBuzzer *buzzer, float *initial_temperatures, uint8_t *sorted_nums){
    uint8_t founded_counter = 0;

    for(uint8_t single_sensor_tries = 0; founded_counter < TEMP_SENSOR_AMOUNT; single_sensor_tries++){
        TemperatureSensorsMeasure(sensors, TEMP_SENSOR_AMOUNT, 0);
        for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
            if(is_in_array(i, sorted_nums, founded_counter))
                continue;
            if(sensors[i].temperature - initial_temperatures[i] > CALIBRATION_TEMP_DELTA){
                sorted_nums[founded_counter] = i;
                founded_counter++;
                single_sensor_tries = 0;
                buzzer->up(buzzer, 300, 600, 30, 30, 1);
                break;
            }
        }
        Delay(3000);
        if(single_sensor_tries > 15){
            buzzer->beep(buzzer, 1600, 100);
            buzzer->beep(buzzer, 800, 100);
            Delay(300);
            buzzer->beep_repeat(buzzer, 500, 100, 400, founded_counter);
            single_sensor_tries = 0;
        }
    }

}

void waiting_cooling_down(DS18B20 *sensors, tBuzzer *buzzer, float *initial_temps, uint8_t delta, uint8_t length){
    float deviation = delta + 1;
    while(deviation > delta){
        TemperatureSensorsMeasure(sensors, length, 0);
        DS18B20_copy_temperature_list(sensors, initial_temps, length);
        deviation = max_temp_deviation(initial_temps, length);
        if(deviation > delta){
            buzzer->down(buzzer, 600, 100, 30, 30, 2);
            Delay(10000);
        }
    }
}

uint8_t Calibration_routine(DS18B20 *sensors, tBuzzer *buzzer, uint64_t *sorted_serials){
    OneWireStatus status;
    uint64_t initial_serials[12] = {0};
    uint8_t temp_order[12] = {0};
    uint8_t sorted_nums[TEMP_SENSOR_AMOUNT] = {0};
    float initial_temperature[TEMP_SENSOR_AMOUNT] = {.0};

    int sensors_amount = OneWire_SearchDevices(sensors[0].ow);
    if(sensors_amount != TEMP_SENSOR_AMOUNT - 1){
        for(uint8_t i = 0; i < 3; i++){
            buzzer->down(buzzer, 500, 300, 30, 30, 1);
            buzzer->up(buzzer, 300, 500, 30, 30, 1);
        }
        return 0;
    }
    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        initial_serials[i] = sensors[0].ow->ids[i].serial_code;
        sensors[i].serialNumber = &sensors[0].ow->ids[i];
    }

    waiting_cooling_down(sensors, buzzer, initial_temperature, CALIBRATION_TEMP_DELTA, TEMP_SENSOR_AMOUNT);
    buzzer->beep(buzzer, 500, 100);
    buzzer->beep(buzzer, 1000, 100);
    RecognitionRoutine(sensors, buzzer, initial_temperature, sorted_nums);
    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        sorted_serials[i] = initial_serials[sorted_nums[i]];
    }
    if(!is_unic_set(sorted_nums, TEMP_SENSOR_AMOUNT)){
        buzzer->mario_underground(buzzer);
        return 0;
    }
    buzzer->tmnt(buzzer);
    return 1;
}

void System_Init(){
    uint64_t sorted_serials[12] = {0};

    SysTick_Config(millisec);
    // RCC_init_MSI();

    gpio_init(LED, General_output, Push_pull, no_pull, Low_speed);

    // gpio_init(EN_PERIPH, General_output, Push_pull, no_pull, Low_speed);
    // gpio_init(UART2_TX, PA2_USART2_TX, Push_pull, pull_up, High_speed);

    tBuzzer buzzer = Buzzer(TIM15, PWM_CH2, BUZZ, PB15_TIM15_CH2);
    buzzer.beep(&buzzer, 400, 200);
    buzzer.beep(&buzzer, 800, 200);
    // if((RCC->CSR & RCC_CSR_IWDGRSTF) || (RCC->CSR & RCC_CSR_WWDGRSTF)){
    //     RCC->CSR |= RCC_CSR_RMVF;
    //     buzzer.down(&buzzer, 400, 100, 30, 30, 3);
    // }
    // IWDG_init(WATCHDOG_PERIOD_MS);

    // volatile uint8_t init_status = RTC_Init();

    // ow = (OneWire){.uart=USART2};

    // for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
    //     sensors[i] = (DS18B20){0};
    //     sensors[i].ow = &ow;
    // }
    // if(init_status) {  // first power on
    //     buzzer.mario(&buzzer);
    // }

    // gpio_state(EN_PERIPH, HIGH);

    // FLASH_read(FLASH_PAGE, FLASH_CONFIG_OFFSET, DS18B20_SERIAL_NUMS, TEMP_SENSOR_AMOUNT);
    // if(DS18B20_SERIAL_NUMS[0] == 0xFFFFFFFFFFFFFFFF){
    //     if(Calibration_routine(sensors, &buzzer, sorted_serials)){
    //         FLASH_write(FLASH_PAGE, FLASH_CONFIG_OFFSET, sorted_serials, TEMP_SENSOR_AMOUNT);
    //         FLASH_read(FLASH_PAGE, FLASH_CONFIG_OFFSET, DS18B20_SERIAL_NUMS, TEMP_SENSOR_AMOUNT);
    //     }
    // }

    // adc = (ADC){
    //     .ADCx = ADC1,
    //     .clk_devider = ADC_ClockDevider_1,
    //     .internal_channels = {
    //         .temp = false,
    //         .vbat = false,
    //         .vref = true
    //     },
    //     .resolution = ADC_12bit,
    //     .mode = ADC_SINGLE_MODE,
    //     .trigger.polarity = ADC_Software_trigger,
    //     .ovrsmpl_ratio = OVRSMPL_32x
    // };
    // ADC_Init(&adc);
    // /*
    //                 6 (50cm)        5 (40cm)        4 (30cm)        3 (20cm)        2 (10cm)        1 (0cm)     VCC  GND
    //                 (CH14, PC5)     (CH15, PB0)     (CH13, PC4)     (CH12, PA7)     (CH10, PA5)     (CH9, PA4)
    // VCC GNS TMP_S
    //                 NC              11 (100cm)      10 (90cm)       9 (80cm)        8 (75cm)        7 (60cm)    VCC  GND
    //                 (CH6, PA1)      (CH1, PC0)      (CH2, PC1)      (CH3, PC2)      (CH4, PC3)      (CH5, PA0)
    // */
    // ADC_InitRegChannel(&adc, CH9, PA4, SMP_92);
    // ADC_InitRegChannel(&adc, CH10, PA5, SMP_92);
    // ADC_InitRegChannel(&adc, CH12, PA7, SMP_92);
    // ADC_InitRegChannel(&adc, CH13, PC4, SMP_92);
    // ADC_InitRegChannel(&adc, CH15, PB0, SMP_92);
    // ADC_InitRegChannel(&adc, CH14, PC5, SMP_92);
    // // ADC_InitRegChannel(&adc, CH6, PA1, SMP_92);
    // ADC_InitRegChannel(&adc, CH1, PC0, SMP_92);
    // ADC_InitRegChannel(&adc, CH2, PC1, SMP_92);
    // ADC_InitRegChannel(&adc, CH3, PC2, SMP_92);
    // ADC_InitRegChannel(&adc, CH4, PC3, SMP_92);
    // ADC_InitRegChannel(&adc, CH5, PA0, SMP_92);
    // ADC_InitRegChannel(&adc, VREF, uninitialized, SMP_92);
    // // ADC_InitRegChannel(&adc, VBAT, uninitialized, SMP_92);
    // // ADC_InitRegChannel(&adc, TEMP, uninitialized, SMP_92);
    // ADC_Enable(&adc);
    // RTC_get_time(&current_rtc);
    // ADC_Start(&adc);
    // TemperatureSensorsMeasure(sensors, TEMP_SENSOR_AMOUNT, 1);
    // ADC_WaitMeasures(&adc, 1000000);

    // // uint32_t vdda = adc.vdda_mvolt;
    // // float internal_temp = ADC_internal_temp(adc.reg_channel_queue[13].result);

    // char str[BUFFER_SIZE] = {0};

    // RTC_auto_wakeup_enable(WAKEUP_PERIOD_SEC);

    // RCC->CRRCR |= RCC_CRRCR_HSI48ON;
    // while(!(RCC->CRRCR & RCC_CRRCR_HSI48RDY));
    // SDMMC_INIT();
    // SDResult result = SD_Init();
    // FAT32t fat32;
    // FAT32_File file;
    // fat32 = FAT32();
    // if(fat32.last_status == OK){

    // }
    // file = fat32.open(&fat32, "text1.txt");
    // if(file.status == OK){
    //     file.append(&file, "hello", 6);
    // }
    // FAT32_File file;
    // if(result == SDR_Success){
    //     fat32 = FAT32();
    //     file = fat32.open(&fat32, "text1.txt");
    //     if(file.status == OK){
    //         // // uint16_t counter = RTC_string_datetime(&current_rtc, str);
    //         // counter += DS18B20_array_to_str(sensors, TEMP_SENSOR_AMOUNT, str, BUFFER_SIZE, counter);
    //         // counter += ADC_array_to_str(&adc, MOISTURE_SENSOR_AMOUNT, str, BUFFER_SIZE, counter);
    //         // uint32_t wrote_count = file.append(&file, str, strlen(str));
    //     }

    // }
    // else {
    //     buzzer.down(&buzzer, 1500, 500, 30, 30, 3);
    // }
    // gpio_state(EN_PERIPH, LOW);
    // stop_cortex();

}


