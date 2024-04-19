#ifndef INC_DS18B20_MISC_H_
#define INC_DS18B20_MISC_H_

#include "stm32l431xx.h"
#include "ds18b20.h"
#include "one_wire.h"

#define TEMP_SENSOR_AMOUNT 12

typedef struct DS18B20_BUS{
    OneWire *ow;
    DS18B20 *sensors;
    uint8_t amount;
    uint64_t *serials;
    void (*greetings)(void);
    void (*notification)(void);
    void (*on_single_registered)(void);
    void (*on_registration_finished)(void);
    void (*on_cooling_down)(void);
    void (*delay_ms)(uint32_t);
} DS18B20_BUS;

OneWireStatus TemperatureSensorsMeasure(DS18B20_BUS *sensors, uint8_t is_sorted);
void RecognitionRoutine(DS18B20_BUS *sensors, float *initial_temperatures, uint8_t *sorted_nums);
uint8_t Calibration_routine(DS18B20_BUS *sensors, uint64_t *sorted_serials);

extern DS18B20_BUS sensors_bus;
#endif