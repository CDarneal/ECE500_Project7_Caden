#include "tmp102.h"
#include "i2c.h"

/* Assumption: TMP102 ADD0 is tied low, giving 7-bit address 0x48.
 * STM32 HAL expects the 7-bit address shifted left by one bit. */
#define TMP102_I2C_ADDR          (0x48U << 1)
#define TMP102_REG_TEMP          0x00U
#define TMP102_REG_CONFIG        0x01U
#define TMP102_CONFIG_OS         0x8000U
#define TMP102_CONFIG_SD         0x0100U
#define TMP102_CONVERSION_TIMEOUT_MS  50U
#define TMP102_I2C_TIMEOUT_MS         100U

static HAL_StatusTypeDef tmp102_read_reg(uint8_t reg, uint8_t *data, uint16_t size)
{
    return HAL_I2C_Mem_Read(&hi2c1, TMP102_I2C_ADDR, reg,
                            I2C_MEMADD_SIZE_8BIT, data, size,
                            TMP102_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef tmp102_write_reg(uint8_t reg, const uint8_t *data, uint16_t size)
{
    return HAL_I2C_Mem_Write(&hi2c1, TMP102_I2C_ADDR, reg,
                             I2C_MEMADD_SIZE_8BIT, (uint8_t *)data, size,
                             TMP102_I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef tmp102_read_config(uint16_t *config)
{
    uint8_t bytes[2];
    HAL_StatusTypeDef status = tmp102_read_reg(TMP102_REG_CONFIG, bytes, 2U);
    if (status == HAL_OK)
    {
        *config = (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    }
    return status;
}

static HAL_StatusTypeDef tmp102_write_config(uint16_t config)
{
    uint8_t bytes[2];
    bytes[0] = (uint8_t)(config >> 8);
    bytes[1] = (uint8_t)(config & 0xFFU);
    return tmp102_write_reg(TMP102_REG_CONFIG, bytes, 2U);
}

HAL_StatusTypeDef tmp102_initialize(void)
{
    uint16_t config;
    HAL_StatusTypeDef status = tmp102_read_config(&config);
    if (status != HAL_OK)
    {
        return status; /* Sensor did not acknowledge/respond. */
    }

    /* Keep the TMP102 in shutdown between measurements; each reading will
       explicitly request a one-shot conversion. Preserve other config bits. */
    config |= TMP102_CONFIG_SD;
    config &= (uint16_t)~TMP102_CONFIG_OS;
    return tmp102_write_config(config);
}

HAL_StatusTypeDef tmp102_read_temperature_raw(uint16_t *raw_temperature)
{
    if (raw_temperature == NULL)
    {
        return HAL_ERROR;
    }

    uint16_t config;
    HAL_StatusTypeDef status = tmp102_read_config(&config);
    if (status != HAL_OK)
    {
        return status;
    }

    /* In shutdown mode, writing OS=1 starts a single conversion. */
    config |= (TMP102_CONFIG_SD | TMP102_CONFIG_OS);
    status = tmp102_write_config(config);
    if (status != HAL_OK)
    {
        return status;
    }

    /* Poll OS until conversion completes instead of relying on a fixed delay. */
    uint32_t start = HAL_GetTick();
    do
    {
        HAL_Delay(1U);
        status = tmp102_read_config(&config);
        if (status != HAL_OK)
        {
            return status;
        }
        if ((config & TMP102_CONFIG_OS) != 0U)
        {
            break;
        }
    } while ((HAL_GetTick() - start) < TMP102_CONVERSION_TIMEOUT_MS);

    if ((config & TMP102_CONFIG_OS) == 0U)
    {
        return HAL_TIMEOUT;
    }

    uint8_t bytes[2];
    status = tmp102_read_reg(TMP102_REG_TEMP, bytes, 2U);
    if (status != HAL_OK)
    {
        return status;
    }

    *raw_temperature = (uint16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    return HAL_OK;
}

HAL_StatusTypeDef tmp102_read_temperature_oneshot(float *temperature_c)
{
    uint16_t raw_temperature;
    HAL_StatusTypeDef status;
    uint16_t raw12;
    int16_t signed_raw;

    if (temperature_c == NULL)
    {
        return HAL_ERROR;
    }

    status = tmp102_read_temperature_raw(&raw_temperature);
    if (status != HAL_OK)
    {
        return status;
    }

    /* TMP102 12-bit temperature data is left-justified in the 16-bit word. */
    raw12 = (uint16_t)(raw_temperature >> 4);
    if ((raw12 & 0x0800U) != 0U)
    {
        signed_raw = (int16_t)((int32_t)raw12 - 4096);
    }
    else
    {
        signed_raw = (int16_t)raw12;
    }

    *temperature_c = (float)signed_raw * 0.0625f;
    return HAL_OK;
}
