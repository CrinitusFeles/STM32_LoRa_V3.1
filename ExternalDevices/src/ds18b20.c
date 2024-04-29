#include "ds18b20.h"
#include "delay.h"
#include "xprintf.h"
#include "dwt.h"

/*
If the DS18B20 is powered by an external supply, the master can issue read time
slots after the Convert T command and the DS18B20 will respond by transmitting
a 0 while the temperature conversion is in progress and a 1 when the conversion
is done. In parasite power mode this notification technique cannot be used
since the bus is pulled high by the strong pullup during the conversion.
*/
OW_Status DS18B20_StartTempMeas(OneWire *ow){
    OW_Status status = OW_Reset(ow);
    status = OW_Write(ow, DS18B20_SKIP_ROM);
    if(status != OW_OK) return status;
    status = OW_Write(ow, DS18B20_CONVERT_TEMP);
    if(status != OW_OK) return status;
    uint8_t answer = 0;
    uint32_t timeout = 150000;
    while(answer == 0){
        status = OW_SendBit(ow, 0, &answer);
        if(status != OW_OK) return status;
        status = OW_Read(ow, &answer);
        if(status != OW_OK) return status;
        timeout--;
        if(timeout == 0) return OW_TIMEOUT;
    }
    return status;
}

OW_Status DS18B20_ReadScratchpad(DS18B20 *sensor){
    OW_Status status = OW_MatchRom(sensor->ow, sensor->serialNumber);
    if(status != OW_OK) return status;
    status = OW_Write(sensor->ow, DS18B20_READ_SCRATCHPAD);
    if(status != OW_OK) return status;
    status = OW_ReadArray(sensor->ow, (uint8_t *)(&(sensor->scratchpad)), 9);
    sensor->temperature = (uint32_t)(sensor->scratchpad.temperature) * 0.0625;
    return status;
}

OW_Status DS18B20_Init(DS18B20 *sensor, OneWire *ow){
    uint8_t devices_on_bus = 0;
    OW_Status status = OW_SearchDevices(ow, &devices_on_bus);
    if(status != OW_OK) return status;
    if(devices_on_bus > 0){
        for(uint8_t i = 0; i < MAXDEVICES_ON_THE_BUS; i++){
            if(ow->ids[i].family != 0){
                sensor[i].ow = ow;
                sensor[i].isConnected = 1;
                sensor[i].serialNumber = &ow->ids[i];  // копируем адрес первого байта ROM
            }
            else break;
        }
    }
    return status;
}

OW_Status DS18B20_ReadROM(DS18B20 *sensor){
    OW_Status status = OW_Reset(sensor->ow);
    if(status != OW_OK) return status;
    status = OW_Write(sensor->ow, DS18B20_READ_ROM);
    if(status != OW_OK) return status;
    return OW_ReadArray(sensor->ow, (uint8_t *)(&sensor->serialNumber), 8);
}

OW_Status DS18B20_ReadTemperature(DS18B20 *sensor){
    OW_Status status = OW_MatchRom(sensor->ow, sensor->serialNumber);
    if(status != OW_OK) return status;
    status = OW_Write(sensor->ow, DS18B20_READ_SCRATCHPAD);
    if(status != OW_OK) return status;
    status = OW_ReadArray(sensor->ow, (uint8_t *)(&(sensor->scratchpad)), 2);
    sensor->temperature = (float)(sensor->scratchpad.temperature >> 4);
    return status;
}

// uint16_t DS18B20_array_to_str(DS18B20 *sensors, size_t length, char *buf, size_t buf_size){
//     for(uint8_t i = 0; i < length; i++){
//         xsprintf(buf + strlen(buf), "%.2f\t", sensors[i].temperature);
//     }
// }

// void DS18B20_copy_temperature_list(DS18B20 *sensors, float *buffer, uint8_t size){
//     for(uint8_t i = 0; i < size; i++){
//         buffer[i] = sensors[i].temperature;
//     }
// }

// void DS18B20_copy_ids_list(DS18B20 *sensors, uint64_t *buffer, uint8_t size){
//     for(uint8_t i = 0; i < size; i++){
//         buffer[i] = sensors[i].serialNumber->serial_code;
//     }
// }