#include "ds18b20_bus.h"

#define CALIBRATION_TEMP_DELTA  3

OneWireStatus TemperatureSensorsMeasure(DS18B20_BUS *bus, uint8_t is_sorted){
    OneWireStatus status;
    DS18B20_StartTempMeas(bus->ow);
    for(uint8_t i = 0; i < bus->amount; i++){
        if(is_sorted){
            status = OneWire_MatchRom(bus->ow, (RomCode*)(&bus->serials[i]));
        } else{
            status = OneWire_MatchRom(bus->ow, bus->sensors[i].serialNumber);
        }
        if(status == ONE_WIRE_OK){
            OneWire_Write(bus->ow, DS18B20_READ_SCRATCHPAD);
            OneWire_ReadArray(bus->ow, (uint8_t *)(&(bus->sensors[i].scratchpad)), 9);
            bus->sensors[i].temperature = (uint32_t)(bus->sensors[i].scratchpad.temperature) * 0.0625;
        }
    }
    return status;
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
void RecognitionRoutine(DS18B20_BUS *bus, float *initial_temperatures,
                        uint8_t *sorted_nums){
    uint8_t founded_counter = 0;
    uint8_t single_sensor_tries = 0;
    for(; founded_counter < bus->amount; single_sensor_tries++){
        TemperatureSensorsMeasure(bus, 0);
        for(uint8_t i = 0; i < bus->amount; i++){
            if(is_in_array(i, sorted_nums, founded_counter))
                continue;
            float delta = bus->sensors[i].temperature - initial_temperatures[i];
            if(delta > CALIBRATION_TEMP_DELTA){
                sorted_nums[founded_counter] = i;
                founded_counter++;
                single_sensor_tries = 0;
                bus->on_single_registered();
                break;
            }
        }
        bus->delay_ms(3000);
        if(single_sensor_tries > 15){
            bus->notification();
            single_sensor_tries = 0;
        }
    }

}

void waiting_cooling_down(DS18B20_BUS *bus, float *initial_temps, uint8_t delta){
    float deviation = delta + 1;
    while(deviation > delta){
        TemperatureSensorsMeasure(bus, 0);
        for(uint8_t i = 0; i < bus->amount; i++){
            initial_temps[i] = bus->sensors[i].temperature;
        }
        deviation = max_temp_deviation(initial_temps, bus->amount);
        if(deviation > delta){
            bus->on_cooling_down();
        }
    }
}

uint8_t Calibration_routine(DS18B20_BUS *bus, uint64_t *sorted_serials){
    // OneWireStatus status;
    uint64_t initial_serials[TEMP_SENSOR_AMOUNT] = {0};
    uint8_t sorted_nums[TEMP_SENSOR_AMOUNT] = {0};
    float initial_temperature[TEMP_SENSOR_AMOUNT] = {.0};

    int sensors_amount = OneWire_SearchDevices(bus->ow);
    if(sensors_amount != TEMP_SENSOR_AMOUNT - 1){
        return 0;
    }
    for(uint8_t i = 0; i < bus->amount; i++){
        initial_serials[i] = bus->ow->ids[i].serial_code;
        bus->sensors[i].serialNumber = &(bus->ow->ids[i]);
    }

    waiting_cooling_down(bus, initial_temperature, CALIBRATION_TEMP_DELTA);
    bus->greetings();
    RecognitionRoutine(bus, initial_temperature, sorted_nums);
    for(uint8_t i = 0; i < TEMP_SENSOR_AMOUNT; i++){
        sorted_serials[i] = initial_serials[sorted_nums[i]];
    }
    if(!is_unic_set(sorted_nums, TEMP_SENSOR_AMOUNT)){
        return 2;
    }
    bus->on_registration_finished();
    return 1;
}
