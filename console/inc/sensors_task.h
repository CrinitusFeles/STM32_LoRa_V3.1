#ifndef INC_SENSORS_TASK_H_
#define INC_SENSORS_TASK_H_

#include "stm32l4xx.h"

void create_calibration_task();
void create_sensors_measure_task(void *pvParams);

typedef struct Args_t{
    uint8_t argc;
    const char * const * argv;
} Args;

#endif