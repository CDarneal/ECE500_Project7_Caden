#include "fram.h"

/* MB85RS64V SPI command set */
#define FRAM_WREN   0x06U
#define FRAM_RDSR   0x05U
#define FRAM_READ   0x03U
#define FRAM_WRITE  0x02U

#define FRAM_CS_LOW()  HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_RESET)
#define FRAM_CS_HIGH() HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET)

extern SPI_HandleTypeDef hspi1;

HAL_StatusTypeDef FRAM_Init(void)
{
    uint8_t status = 0U;

    /* WP is active low on the board; drive it high to permit writes. */
    HAL_GPIO_WritePin(SPI1_WP_GPIO_Port, SPI1_WP_Pin, GPIO_PIN_SET);
    FRAM_CS_HIGH();

    return FRAM_ReadStatus(&status);
}

HAL_StatusTypeDef FRAM_ReadStatus(uint8_t *status)
{
    uint8_t command = FRAM_RDSR;
    HAL_StatusTypeDef result;

    if (status == NULL)
    {
        return HAL_ERROR;
    }

    FRAM_CS_LOW();
    result = HAL_SPI_Transmit(&hspi1, &command, 1U, HAL_MAX_DELAY);
    if (result == HAL_OK)
    {
        result = HAL_SPI_Receive(&hspi1, status, 1U, HAL_MAX_DELAY);
    }
    FRAM_CS_HIGH();
    return result;
}

HAL_StatusTypeDef FRAM_Read(uint16_t address, uint8_t *data, uint16_t length)
{
    uint8_t command[3];
    HAL_StatusTypeDef result;

    if ((data == NULL) || (length == 0U) || (address >= FRAM_SIZE) ||
        (((uint32_t)address + (uint32_t)length) > FRAM_SIZE))
    {
        return HAL_ERROR;
    }

    command[0] = FRAM_READ;
    command[1] = (uint8_t)(address >> 8);
    command[2] = (uint8_t)(address & 0xFFU);

    FRAM_CS_LOW();
    result = HAL_SPI_Transmit(&hspi1, command, 3U, HAL_MAX_DELAY);
    if (result == HAL_OK)
    {
        result = HAL_SPI_Receive(&hspi1, data, length, HAL_MAX_DELAY);
    }
    FRAM_CS_HIGH();
    return result;
}

HAL_StatusTypeDef FRAM_Write(uint16_t address, const uint8_t *data, uint16_t length)
{
    uint8_t command[3];
    HAL_StatusTypeDef result;

    if ((data == NULL) || (length == 0U) || (address >= FRAM_SIZE) ||
        (((uint32_t)address + (uint32_t)length) > FRAM_SIZE))
    {
        return HAL_ERROR;
    }

    /* Write enable must be a separate CS-delimited transaction. */
    command[0] = FRAM_WREN;
    FRAM_CS_LOW();
    result = HAL_SPI_Transmit(&hspi1, command, 1U, HAL_MAX_DELAY);
    FRAM_CS_HIGH();
    if (result != HAL_OK)
    {
        return result;
    }

    command[0] = FRAM_WRITE;
    command[1] = (uint8_t)(address >> 8);
    command[2] = (uint8_t)(address & 0xFFU);

    FRAM_CS_LOW();
    result = HAL_SPI_Transmit(&hspi1, command, 3U, HAL_MAX_DELAY);
    if (result == HAL_OK)
    {
        result = HAL_SPI_Transmit(&hspi1, (uint8_t *)data, length, HAL_MAX_DELAY);
    }
    FRAM_CS_HIGH();
    return result;
}
