#ifndef TMP102_H
#define TMP102_H
#include "main.h"
#include <stdint.h>
HAL_StatusTypeDef tmp102_initialize(void);
HAL_StatusTypeDef tmp102_read_temperature_oneshot(float *temperature_c);
HAL_StatusTypeDef tmp102_read_temperature_raw(uint16_t *raw_temperature);
#endif
