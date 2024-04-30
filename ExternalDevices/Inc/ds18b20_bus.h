#ifndef INC_DS18B20_MISC_H_
#define INC_DS18B20_MISC_H_

#include "stm32l431xx.h"
#include "ds18b20.h"
#include "one_wire.h"

#define TEMP_SENSOR_AMOUNT 12

typedef struct DS18B20_BUS{
    OneWire *ow;
    DS18B20 *sensors;
    uint8_t connected_amount;
    uint8_t found_amount;
    uint8_t is_calibrated;
    uint64_t *serials;
    void (*greetings)(void);
    void (*notification)(int);
    void (*on_single_registered)(void);
    void (*on_registration_finished)(void);
    void (*on_cooling_down)(void);
    void (*delay_ms)(uint32_t);
} DS18B20_BUS;

typedef enum{
  DS18B20_OK = 0,
  DS18B20_EMPTY_BUS = 1,
  DS18B20_TIMEOUT = 2,
  DS18B20_ROM_FINDING_ERROR = 3,
  DS18B20_INCORRECT_SENSORS_AMOUNT = 4,
  DS18B20_RECOGNITION_FAILED = 5,
} DS18B20_BUS_Status;

OW_Status TemperatureSensorsMeasure(DS18B20_BUS *sensors, uint8_t is_sorted);
DS18B20_BUS_Status Calibration_routine(DS18B20_BUS *sensors);

extern DS18B20_BUS sensors_bus;
#endif