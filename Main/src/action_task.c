#include "action_task.h"
#include "system_config.h"
#include "ff.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ds18b20_bus.h"
#include "low_power.h"
#include "xprintf.h"
#include "rtc.h"
#include "gsm.h"
#include "adc.h"
#include "main.h"
#include "console_utils.h"
#include <string.h>


#define TASK_SIZE       configMINIMAL_STACK_SIZE * 3
#define UNUSED(x)       (void)(x)
#define LOG_FILENAME    "measures.log"
#define BIN_FILENAME    "measures.bin"
#define MEASURES_SIZE   (TEMP_SENSOR_AMOUNT * 2 + 18)


typedef union Measures{
    uint8_t data[MEASURES_SIZE];
    struct {
        uint16_t label;
        uint16_t module_id;
        uint32_t timestamp;
        uint32_t measure_num;
        uint16_t voltage;
        uint16_t temperatures[TEMP_SENSOR_AMOUNT];
        uint32_t crc32;
    };
} Measures;


StackType_t xStack_action [TASK_SIZE];
StaticTask_t xTaskBuffer_action;
TaskHandle_t action_task;

OW_Status ow_status = OW_OK;
UINT written_count = 0;
UINT send_size = 0;
UINT read_count = 0;

void log_error(char *msg){
    uint16_t sd_ptr = 0;
    UINT _written_count = 0;
    f_open(&file, "errors.log", FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    f_lseek(&file, file.obj.objsize);
    sd_ptr += RTC_string_datetime(file_buff + sd_ptr);
    sd_ptr += xsprintf(file_buff + sd_ptr, "  ");
    sd_ptr += xsprintf(file_buff + sd_ptr, msg);
    f_write(&file, file_buff, strlen(file_buff), &_written_count);
    f_close(&file);
}


uint8_t GSM_Routine(UINT read_amount){
    uint8_t status = 0;
    GSM_TogglePower(&sim7000g);
    vTaskDelay(10000);
    if(GSM_SendCMD(&sim7000g, "AT") == 0){  // waiting for power on
        for(uint16_t i = 0; i < 10000; i++){
            if(sim7000g.status.sim_status) break;
            vTaskDelay(1);
        }
    }
    GSM_DisableEcho(&sim7000g);
    GSM_GetVBAT(&sim7000g);
    if(GSM_InitGPRS(&sim7000g) == 0){
        char port[6] = {0};
        xsprintf(port, "%d", system_config.port);
        if(GSM_OpenConnection(&sim7000g, system_config.ip, port)){
            GSM_SendFile(&sim7000g, BIN_FILENAME, read_amount);
        } else {
            log_error("Failed connect to server\n");
            status = 1;
        }
    } else {
        log_error("Failed init GPRS\n");
        status = 1;
    }
    // if(status = 1) vTaskDelay(50);
    GSM_PowerOFF(&sim7000g);
    for(uint16_t i = 0; i < 1000; i++){
        if(sim7000g.status.pwr_status == 0) break;
        vTaskDelay(1);
    }
    return status;
}

uint32_t calc_crc(uint32_t *buffer, uint32_t word_amount){
    register uint8_t ptr;
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    CRC->CR = 1;
    for (ptr = 0; ptr < word_amount; ptr++) {
        CRC->DR = *buffer++;
    }
    return CRC->DR;
}

void write_txt_log(){
    uint16_t sd_ptr = 0;
    FRESULT res = f_open(&file, LOG_FILENAME, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    if(res == FR_OK){
        if (f_size(&file) == 0) {
            sd_ptr += xsprintf(file_buff + sd_ptr, "Module id: %d\n\n", system_config.module_id);
            for (uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++) {
                sd_ptr += xsprintf(file_buff + sd_ptr, "T%02d: [0x%016llX]\n", i + 1,
                                    sensors_bus.is_calibrated ? sensors_bus.serials[i]
                                                                : sensors_bus.sensors[i].serialNumber->serial_code);
            }
            sd_ptr += xsprintf(file_buff + sd_ptr, "\n");
            sd_ptr += xsprintf(file_buff + sd_ptr, "%-25s", "Timestamp");
            for (uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++) {
                sd_ptr += xsprintf(file_buff + sd_ptr, "T%02d    ", i + 1);
            }
            sd_ptr += xsprintf(file_buff + sd_ptr, "Vref\n");
        }

        sd_ptr += RTC_string_datetime(file_buff + sd_ptr);
        sd_ptr += xsprintf(file_buff + sd_ptr, "  ");
        for (uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++) {
            sd_ptr += xsprintf(file_buff + sd_ptr, "%.2f  ", sensors_bus.sensors[i].temperature);
        }
        sd_ptr += xsprintf(file_buff + sd_ptr, "%d\n", adc.vdda_mvolt);
        f_lseek(&file, file.obj.objsize);
        f_write(&file, file_buff, sd_ptr, &written_count);
    }
    f_close(&file);
}


void write_bin_log(){
    Measures measures;

    measures.label = 0x0FF1;
    measures.timestamp = RTC_EncodeDateTime(&current_rtc);
    for (uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++) {
        measures.temperatures[i] = sensors_bus.sensors[i].scratchpad.temperature;
    }
    measures.crc32 = calc_crc((uint32_t*)measures.data, (MEASURES_SIZE - 4) / 4);
    measures.voltage = adc.vdda_mvolt;
    measures.measure_num = RTC->BKP0R;
    measures.module_id = system_config.module_id;
    FRESULT res = f_open(&file, BIN_FILENAME, FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
    if(res == FR_OK){
        f_lseek(&file, file.obj.objsize);
        res = f_write(&file, measures.data, MEASURES_SIZE, &written_count);
        if(res == FR_OK){
            send_size += written_count;
        }
    }
    f_close(&file);
}


void ACTION_TASK(void *pvParameters){
    UNUSED(pvParameters);
    sensors_bus.power_on();
    vTaskDelay(50);
    // ADC_Start(&adc);
    if (sensors_bus.found_amount == 0) {
        ow_status = OW_EMPTY_BUS;
    } else {
        ow_status = TemperatureSensorsMeasure(&sensors_bus, sensors_bus.is_calibrated);
    }
    // ADC_WaitMeasures(&adc, 10000);
    sensors_bus.power_off();

    send_size = RTC->BKP1R;
    write_txt_log();
    write_bin_log();

    switch (ow_status) {
    case OW_OK:
        break;
    case OW_EMPTY_BUS:
        log_error("Empty bus\n");
        break;
    case OW_TIMEOUT:
        log_error("Timeout\n");
        break;
    case OW_ROM_FINDING_ERROR:
        log_error("ROM_FINDING_ERROR\n");
        break;
    default:
        log_error("ERROR\n");
        break;
    }

    write_single_bkp_reg(BCKP_MEAS_AMOUNT, RTC->BKP2R + 1);
    if(RTC->BKP2R > (uint32_t)system_config.modem_period){
        if(GSM_Routine(send_size)){

        } else {
            send_size = 0;
            write_single_bkp_reg(BCKP_MEAS_AMOUNT, 0);
        }
    }
    write_single_bkp_reg(BCKP_SEND_SIZE, send_size);

    f_mount(0, "", 1);
    xprintf("\n\rGo to sleep\n\r\n\r");
    RTC_auto_wakeup_enable(system_config.wakeup_period);
    gpio_state(EN_SD, HIGH);
    gpio_state(EN_LORA, HIGH);
    stop_cortex();

    vTaskDelete(NULL);
}

void create_action_task(){
    action_task = xTaskCreateStatic(
        ACTION_TASK, "ACTION_TASK", TASK_SIZE,
        NULL, 0, xStack_action, &xTaskBuffer_action
    );
}