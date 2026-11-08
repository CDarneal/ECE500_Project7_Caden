#ifndef FRAM_H
#define FRAM_H

#include "main.h"
#include <stdint.h>

#define FRAM_SIZE       8192U
#define FRAM_START_ADDR 0x0000U
#define FRAM_END_ADDR   0x1FFFU

HAL_StatusTypeDef FRAM_Init(void);
HAL_StatusTypeDef FRAM_ReadStatus(uint8_t *status);
HAL_StatusTypeDef FRAM_Read(uint16_t address, uint8_t *data, uint16_t length);
HAL_StatusTypeDef FRAM_Write(uint16_t address, const uint8_t *data, uint16_t length);

#endif /* FRAM_H */
