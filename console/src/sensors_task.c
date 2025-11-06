#include "FreeRTOS.h"
#include "task.h"
#include "sensors_task.h"
#include "ff.h"
#include "xprintf.h"
#include "adc.h"
#include <string.h>
#include "ds18b20_bus.h"
#include "system_config.h"
#include "flash.h"
#include "rtc.h"

#define TASK_SIZE               configMINIMAL_STACK_SIZE * 4
StaticTask_t xTaskBuffer;
StackType_t xStack [TASK_SIZE];

#define UNUSED(x) (void)(x)


void CALIBRATION_TASK(void *pvParameters){
    UNUSED(pvParameters);
    if(sensors_bus.is_calibrated){
        xprintf("Temperature sensors already calibrated."\
                "Recalibration process started\n");
    }
    sensors_bus.power_on();
    DS18B20_BUS_Status status = Calibration_routine(&sensors_bus);
    if(status != (DS18B20_BUS_Status)OW_OK){
        xprintf("Calibration failed: %d\n", status);
        sensors_bus.power_off();
        return;
    }
    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        xprintf("T%02d id: [0x%016llX]\n", i + 1, sensors_bus.serials[i]);
    }
    if(system_config.auto_save_config){
        if(save_system_config_to_FLASH(&system_config) != FLASH_OK)
            xprintf("Saving configuration failed!\n");
        if(save_config_to_SD(&system_config, SYSTEM_CONFIG_PATH, config_json) != CONFIG_OK)
            xprintf("Cant save config to SD\n");
    }
    sensors_bus.power_off();
    vTaskDelete(NULL);
}

void create_calibration_task(){
    xTaskCreateStatic(
        CALIBRATION_TASK, "TEMP_CALIB", TASK_SIZE,
        NULL, 2, xStack, &xTaskBuffer
    );
}

void SENSORS_MEASURE_TASK(void *pvParameters){
    Args *args;
    args = (Args *)(pvParameters);
    #define TEMP_ONLY   1 << 0
    #define ADC_ONLY    1 << 1
    uint8_t save_to_sd = 0;
    uint8_t meas_mode = 3;  // 3 - adc and temp; 2 - adc only; 1 - temp only
    long meas_amount = 1;
    long meas_period = 0;
    OW_Status status = OW_OK;
    int buffer_ptr = 0;
    FSIZE_t file_size = 0;
    UINT written_count = 0;
    FRESULT res;

    for(uint8_t i = 1; i < args->argc; i++){
        if(strcmp(args->argv[i], "-m") == 0){
            if(i < args->argc - 1){
                if(strcmp(args->argv[i + 1], "ADC") == 0){
                    meas_mode = 2;
                } else if(strcmp(args->argv[i + 1], "TEMP") == 0){
                    meas_mode = 1;
                } else {
                    xprintf("Incorrect mode flag: %s\n", args->argv[i + 1]);
                    break;
                }
            }
        } else if(strcmp(args->argv[i], "-f") == 0){
            if(i < args->argc - 1){
                res = f_open(&file, args->argv[i + 1], FA_OPEN_ALWAYS | FA_READ | FA_WRITE);
                if(res == FR_OK){
                    save_to_sd = 1;
                } else {
                    xprintf("Can not open file %s\n", args->argv[i + 1]);
                    break;
                }
            }
        } else if(strcmp(args->argv[i], "-n") == 0){
            if(i < args->argc - 1){
                if(xatoi((char*)args->argv[i + 1], &meas_amount) == 0){
                    xprintf("Incorrect measure amount flag: %s\n", args->argv[i + 1]);
                    break;
                }
            }
        } else if(strcmp(args->argv[i], "-t") == 0){
            if(i < args->argc - 1){
                if(xatoi((char*)args->argv[i + 1], &meas_period) == 0){
                    xprintf("Incorrect measure period flag: %s\n", args->argv[i + 1]);
                    break;
                }
            }
        }
    }

    if(meas_mode & TEMP_ONLY){
        if(!sensors_bus.is_calibrated){
            xprintf("Start measure uncalibrated sensors\n");
        }
    }

    for(long meas_num = 0; meas_num < meas_amount; meas_num++){
        sensors_bus.power_on();
        if(meas_mode & TEMP_ONLY){
            if(sensors_bus.found_amount == 0){
                xprintf("Start searching sensors\n");
                status = OW_SearchDevices(sensors_bus.ow, &sensors_bus.found_amount);
                if(status != OW_OK){
                    if(status == OW_EMPTY_BUS){
                        xprintf("Not found connected sensors. Check the power supply.\n");
                    } else if(status == OW_ROM_FINDING_ERROR){
                        xprintf("OneWire reading error\n");
                    } else if(status == OW_TIMEOUT){
                        xprintf("OneWire TIMEOUT\n");
                    }
                    sensors_bus.power_off();
                    break;
                }
                for(uint8_t i = 0; i < sensors_bus.connected_amount; i++){
                    sensors_bus.sensors[i].serialNumber = &(sensors_bus.ow->ids[i]);
                }
                if(status == OW_OK && sensors_bus.found_amount > 0){
                    xprintf("Founded sensors: %d\n", sensors_bus.found_amount + 1);
                } else {
                    if(status == OW_EMPTY_BUS){
                        xprintf("Not found connected sensors. Check the power supply.\n");
                    } else if(status == OW_ROM_FINDING_ERROR){
                        xprintf("OneWire reading error\n");
                    } else if(status == OW_TIMEOUT){
                        xprintf("OneWire TIMEOUT\n");
                    }
                    sensors_bus.power_off();
                    break;
                }
            }
            status = TemperatureSensorsMeasure(&sensors_bus, sensors_bus.is_calibrated);
        }
        if(meas_mode & ADC_ONLY){
            vTaskDelay(50);
            ADC_Start(&adc);
            if(ADC_WaitMeasures(&adc, 1000000)){
                xprintf("ADC timeout\n");
            }
        }
        sensors_bus.power_off();
        memset(file_buff, 0, FILE_BUFFER);
        buffer_ptr = 0;
        if(save_to_sd){
            file_size = f_size(&file);
        }
        if(meas_mode & TEMP_ONLY){
            if(status != OW_OK){
                if(status == OW_EMPTY_BUS){
                    xprintf("Not found connected sensors. "\
                            "Check the power supply.\n");
                } else if(status == OW_ROM_FINDING_ERROR){
                    xprintf("OneWire reading error\n");
                } else if(status == OW_TIMEOUT){
                    xprintf("OneWire TIMEOUT\n");
                }
                break;
            }
            if(meas_num == 0 && file_size == 0){
                buffer_ptr += xsprintf(file_buff + buffer_ptr, "Module id: %d\n\n", system_config.module_id);
                for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
                    buffer_ptr += xsprintf(file_buff + buffer_ptr, "T%02d: [0x%016llX]\n", i + 1,
                                        sensors_bus.is_calibrated ?
                                        sensors_bus.serials[i] :
                                        sensors_bus.sensors[i].serialNumber->serial_code);
                }
                buffer_ptr += xsprintf(file_buff + buffer_ptr, "\n");
            }
        }
        if(meas_num == 0 && file_size == 0){
            buffer_ptr += xsprintf(file_buff + buffer_ptr, "%-25s", "Timestamp");
        }

        if(meas_mode & TEMP_ONLY){
            if(meas_num == 0 && file_size == 0){
                for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
                    buffer_ptr += xsprintf(file_buff + buffer_ptr, "T%02d    ", i + 1);
                }
            }
        }

        if(meas_mode & ADC_ONLY){
            if(meas_num == 0 && file_size == 0){
                for(uint8_t i = 0; i < 11; i++){
                    buffer_ptr += xsprintf(file_buff + buffer_ptr, "A%02d   ", i + 1);
                }
                buffer_ptr += xsprintf(file_buff + buffer_ptr, "VDDA");
            }
        }
        if(meas_num == 0 && file_size == 0){
            buffer_ptr += xsprintf(file_buff + buffer_ptr, "\n");
        }

        buffer_ptr += RTC_string_datetime(file_buff + buffer_ptr);
        buffer_ptr += xsprintf(file_buff + buffer_ptr, "  ");

        if(meas_mode & TEMP_ONLY){
            for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
                buffer_ptr += xsprintf(file_buff + buffer_ptr, "%.2f  ",
                                        sensors_bus.sensors[i].temperature);
            }
        }
        if(meas_mode & ADC_ONLY){
            for(uint8_t i = 0; i < 11; i++){
                buffer_ptr += xsprintf(file_buff + buffer_ptr, "%-4d  ",
                                        adc.reg_channel_queue[i].result_mv);
            }
            buffer_ptr += xsprintf(file_buff + buffer_ptr, "%-4d", adc.vdda_mvolt);
        }
        buffer_ptr += xsprintf(file_buff + buffer_ptr, "\n");

        xprintf(file_buff);

        if(save_to_sd){
            res = f_lseek(&file, file_size);
            res = f_write(&file, file_buff, buffer_ptr, &written_count);
            if(res != FR_OK || (int)written_count < buffer_ptr){
                xprintf("Failed to write to SD card\n");
            }
            f_close(&file);
        }
        vTaskDelay(meas_period > 0 ? meas_period * 1000 : 1);
    }
    vTaskDelete(NULL);
}

void create_sensors_measure_task(void *pvParams){
    xTaskCreateStatic(
        SENSORS_MEASURE_TASK, "SENS_MEASURE", TASK_SIZE,
        pvParams, 2, xStack, &xTaskBuffer
    );
}