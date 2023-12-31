#include "ds18b20.h"
#include "delay.h"
#include "stdio.h"
#include "math.h"
// #include "xprintf.h"


uint8_t DS18B20_StartTempMeas(OneWire *ow){
    OneWire_Reset(ow);
    OneWire_Write(ow, DS18B20_SKIP_ROM);
    // OneWire_MatchRom(ow, rom);
    OneWire_Write(ow, DS18B20_CONVERT_TEMP);
    uint8_t answer = 0;
    uint8_t timeout_ms_counter = 0;
    // while(answer == 0 && timeout_ms_counter < DS18B20_CONVERTION_TIMEOUT_MS){
    //     answer = OneWire_SendBit(ow, 0);
    //     timeout_ms_counter = GetMili();
    // }

    // if(answer > 0) return 0;
    Delay(300);
    return 0;
    // return 1;
}

uint8_t DS18B20_ReadScratchpad(DS18B20 *sensor){
    OneWireStatus status = OneWire_MatchRom(sensor->ow, sensor->serialNumber);
    if(status == ONE_WIRE_OK){
        OneWire_Write(sensor->ow, DS18B20_READ_SCRATCHPAD);
        OneWire_ReadArray(sensor->ow, (uint8_t *)(&(sensor->scratchpad)), 9);
        sensor->temperature = (uint32_t)(sensor->scratchpad.temperature) * 0.0625;
    }
    return status;
}

uint8_t DS18B20_Init(DS18B20 *sensor, OneWire *ow){
    int8_t devices_on_bus = OneWire_SearchDevices(ow);
    if(devices_on_bus > 0){
        for(uint8_t i = 0; i < MAXDEVICES_ON_THE_BUS; i++){
            if(ow->ids[i].family != 0){
                sensor[i].ow = ow;
                sensor[i].isConnected = 1;
                sensor[i].serialNumber = &ow->ids[i];  // копируем адрес первого байта ROM
            }
            else break;
        }
        return 1;
    }
    return 0;
}

void DS18B20_ReadROM(DS18B20 *sensor){
    if(OneWire_Reset(sensor->ow) == ONE_WIRE_EMPTY_BUS) return;
    OneWire_Write(sensor->ow, DS18B20_READ_ROM);
    OneWire_ReadArray(sensor->ow, (uint8_t *)(&sensor->serialNumber), 8);
}

uint16_t DS18B20_ReadTemperature(DS18B20 *sensor){
    OneWireStatus status = OneWire_MatchRom(sensor->ow, sensor->serialNumber);
    if(status == ONE_WIRE_OK){
        OneWire_Write(sensor->ow, DS18B20_READ_SCRATCHPAD);
        OneWire_ReadArray(sensor->ow, (uint8_t *)(&(sensor->scratchpad)), 2);
        sensor->temperature = (float)(sensor->scratchpad.temperature >> 4);
    }
    return (uint16_t)(sensor->scratchpad.temperature);
}

int float_to_str(char *buff, float value){
    char *tmpSign = (value < 0) ? "-" : "";
    float tmpVal = (value < 0) ? -value : value;
    int tmpInt1 = tmpVal;                  // Get the integer
    float tmpFrac = tmpVal - tmpInt1;      // Get fraction
    int tmpInt2 = trunc(tmpFrac * 100);
    return sprintf(buff, "%s%d.%02d\t", tmpSign, tmpInt1, tmpInt2);
}

uint16_t DS18B20_array_to_str(DS18B20 *sensors, size_t length, char *buf, size_t buf_size, uint16_t offset){
    uint16_t b_end = offset;
    for(uint16_t i = 0; i < buf_size; i++){
        if(buf[i + offset] == 0){
            b_end += i;
            break;
        }
    }
    uint16_t wrote_count = 0;
    for(uint8_t i = 0; i < length; i++){
        // xsprintf(buf + b_end + wrote_count, "%.2f\t", sensors[i].temperature);
        int size = float_to_str(buf + b_end + wrote_count, sensors[i].temperature);
        if(size > 0)
            wrote_count += size;
        else
            Delay(10000);
    }
    return wrote_count;
}

void DS18B20_copy_temperature_list(DS18B20 *sensors, float *buffer, uint8_t size){
    for(uint8_t i = 0; i < size; i++){
        buffer[i] = sensors[i].temperature;
    }
}

void DS18B20_copy_ids_list(DS18B20 *sensors, uint64_t *buffer, uint8_t size){
    for(uint8_t i = 0; i < size; i++){
        buffer[i] = sensors[i].serialNumber->serial_code;
    }
}