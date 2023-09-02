#include "main.h"
#include "cli.h"
#include "rtc.h"
#include "string.h"
#include "low_power.h"
#include "global_variables.h"
#include "gsm.h"

void CMD_Parser(SX126x *driver, CommandStruct *pkt){
    RTC_struct_brief rtc_data;
    uint16_t measured_data[24] = {0};
    switch (pkt->cmd){
        case ECHO:{
            uint8_t target_id = pkt->target_id;
            pkt->target_id = pkt->sender_id;
            pkt->sender_id = target_id;
            SX126x_SendData(driver, (uint8_t *)(pkt), pkt->arg_len + 5);
            break;
        }
        case GET_RADIO_QUALITY:
            SX126x_SendData(driver, &driver->rssi, 1);
            break;
        case SET_PIN_STATE:
            gpio_state(LED, pkt->args[1]);
            break;
        case SET_WAKEUP_PERIOD:
            if(pkt->arg_len == 2){
                write_single_bkp_reg(10, (pkt->args[0] << 8) | pkt->args[1]);
            }
            break;
        case GET_RTC:  // year, weak_day, month, day, hour, min, sec
            RTC_get_time(&rtc_data);
            SX126x_SendData(driver, (uint8_t*)(&rtc_data), 7);
            break;
        case SET_TIME:  // hour, min, sec
            memcpy((uint8_t*)(&rtc_data) + 4, pkt->args, 3);
            RTC_set_time(RTC_struct_brief_time_converter(&rtc_data));
            break;
        case SET_DATE:  // year, weak_day, month, day
            memcpy((uint8_t*)(&rtc_data), pkt->args, 4);
            RTC_set_date(RTC_struct_brief_date_converter(&rtc_data));
            break;
        case SET_RTC:  // year, weak_day, month, day, hour, min, sec
            memcpy((uint8_t*)(&rtc_data), pkt->args, 7);
            RTC_data_update(&rtc_data);
            break;
        case SINGLE_MEASURE:
            gpio_state(TEMP_SENSOR_PWR_EN, HIGH);
            gpio_state(SOIL_SENSOR_PWR_EN, HIGH);
            uint8_t status = DS18B20_StartTempMeas(&ow);
            for(uint8_t i = 0; i < 12; i++)
                status += DS18B20_ReadScratchpad(&sensors[i]);
            ADC_Start(&adc);
            while(adc.measure_process);
            for(uint8_t i = 0; i < 12; i++){
                measured_data[i] = adc.reg_channel_queue[i].result;
                measured_data[i + 12] = sensors[i].scratchpad.temperature;
            }
            measured_data[11] = adc.vdda_mvolt;
            gpio_state(TEMP_SENSOR_PWR_EN, LOW);
            gpio_state(SOIL_SENSOR_PWR_EN, LOW);
            SX126x_SendData(driver, (uint8_t*)(&measured_data), 48);
            break;
        case GET_MEASURED_DATA:
            for(uint8_t i = 0; i < 12; i++){
                measured_data[i] = adc.reg_channel_queue[i].result;
                measured_data[i + 12] = sensors[i].scratchpad.temperature;
            }
            measured_data[12] = adc.vdda_mvolt;
            SX126x_SendData(driver, (uint8_t*)(&measured_data), 48);
            break;
        case GET_BATTERY:
            SX126x_SendData(driver, (uint8_t*)(&adc.vdda_mvolt), 2);
            break;
        case GO_SLEEP:
            if(pkt->arg_len == 3){
                uint8_t waiting_time_sec = pkt->args[0];
                uint16_t sleep_time_sec = (pkt->args[1] << 8) | pkt->args[2];
                if(sim7000g.status.gprs_connected){
                    GSM_CloseConnections(&sim7000g);
                    GSM_PowerOFF(&sim7000g);
                }
                SX126x_SetSleep(&SX1268);
                Delay(waiting_time_sec * 1000);
                if(sleep_time_sec > 0){
                    RTC_auto_wakeup_enable(sleep_time_sec);
                    stop_cortex();
                }
            }
            break;
        case MY_PERIODIC_DATA:  // rtc[7], measured_data[48], signal_params[3]
            RTC_get_time(&current_rtc);
            uint8_t *gs_buffer_ptr;
            uint16_t *buffer_counter_ptr;
            if(pkt->sender_id == 2) {
                gs_buffer_ptr = gs2_buffer;
                buffer_counter_ptr = (uint16_t*)(&gs2_buf_ptr);
            } else if(pkt->sender_id == 3) {
                gs_buffer_ptr = gs3_buffer;
                buffer_counter_ptr = (uint16_t*)(&gs3_buf_ptr);
            } else break;
            if((*buffer_counter_ptr) + sizeof(current_rtc) + pkt->arg_len + 3 < sizeof(data_buffer)){
                memcpy(gs_buffer_ptr + (*buffer_counter_ptr), &current_rtc, sizeof(current_rtc));
                (*buffer_counter_ptr) += sizeof(current_rtc);
                memcpy(gs_buffer_ptr + (*buffer_counter_ptr), pkt->args, pkt->arg_len);
                (*buffer_counter_ptr) += pkt->arg_len;
                uint8_t signal[3] = {SX1268.signal_rssi, SX1268.snr, SX1268.rssi};
                memcpy(gs_buffer_ptr + (*buffer_counter_ptr), signal, 3);
                (*buffer_counter_ptr) += 3;
            }
            uint32_t time_stamp = (current_rtc.hours << 16) | (current_rtc.minutes << 8 ) | current_rtc.seconds;
            write_single_bkp_reg(pkt->sender_id, time_stamp);
            // write_single_bkp_reg()
            break;
        default:
            break;
    }
}