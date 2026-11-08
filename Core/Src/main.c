/* USER CODE BEGIN Header */

/**

  ******************************************************************************

  * @file           : main.c

  * @brief          : Main program body

  ******************************************************************************

  * @attention

  *

  * Copyright (c) 2026 STMicroelectronics.

  * All rights reserved.

  *

  * This software is licensed under terms that can be found in the LICENSE file

  * in the root directory of this software component.

  * If no LICENSE file comes with this software, it is provided AS-IS.

  *

  ******************************************************************************

  */

/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/

#include "main.h"



/* Private includes ----------------------------------------------------------*/

/* USER CODE BEGIN Includes */

#include "i2c.h"

#include "tmp102.h"

#include "fram.h"

#include <stdio.h>

#include <string.h>

#include <ctype.h>

#include <stdint.h>

/* USER CODE END Includes */



/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */



/* USER CODE END PTD */



/* Private define ------------------------------------------------------------*/

/* USER CODE BEGIN PD */



/* USER CODE END PD */



/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */



/* USER CODE END PM */



/* Private variables ---------------------------------------------------------*/

I2C_HandleTypeDef hi2c1;



SPI_HandleTypeDef hspi1;



UART_HandleTypeDef huart2;



/* USER CODE BEGIN PV */

#define LOGGER_META_START_ADDR  0x0000U
#define LOGGER_DATA_START_ADDR  0x0004U
#define LOGGER_RECORD_SIZE      2U
#define LOGGER_UART_TIMEOUT_MS  5U

/* Metadata stores first and last record start addresses (inclusive).
 * Two zero pointers indicate an empty FRAM log. */
static uint16_t logger_start_address = 0U;
static uint16_t logger_last_address = 0U;
static uint8_t logger_is_running = 0U;
static uint8_t logger_metadata_valid = 1U;
static uint32_t logger_last_sample_tick = 0U;

static char logger_command_buffer[32];

static uint8_t logger_command_length = 0U;

/* USER CODE END PV */



/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

static void MX_GPIO_Init(void);

static void MX_USART2_UART_Init(void);

static void MX_I2C1_Init(void);

static void MX_SPI1_Init(void);

/* USER CODE BEGIN PFP */

static void Logger_LoadMetadata(void);

static HAL_StatusTypeDef Logger_SaveMetadata(void);

static void Logger_ProcessCommand(char *command);

static void Logger_TakeSample(void);

static void Logger_PrintHelp(void);

static void Logger_PrintStatus(void);

static void Logger_DumpCurrentSession(void);
static HAL_StatusTypeDef Logger_ClearMemory(void);
static void Logger_PrintRawTemperature(uint16_t raw_temperature);

/* USER CODE END PFP */



/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */



/* USER CODE END 0 */



/**

  * @brief  The application entry point.

  * @retval int

  */

int main(void)

{



  /* USER CODE BEGIN 1 */



  /* USER CODE END 1 */



  /* MCU Configuration--------------------------------------------------------*/



  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */

  HAL_Init();



  /* USER CODE BEGIN Init */



  /* USER CODE END Init */



  /* Configure the system clock */

  SystemClock_Config();



  /* USER CODE BEGIN SysInit */



  /* USER CODE END SysInit */



  /* Initialize all configured peripherals */

  MX_GPIO_Init();

  MX_USART2_UART_Init();

  MX_I2C1_Init();

  MX_SPI1_Init();

  /* USER CODE BEGIN 2 */

  printf("\r\n========================================\r\n");

  printf(" ECE 500 - PROJECT 7\r\n");

  printf(" TMP102 / SPI FRAM Temperature Logger\r\n");

  printf("========================================\r\n");

  printf("System clock: %lu Hz\r\n", (unsigned long)HAL_RCC_GetHCLKFreq());

  printf("I2C1: PB8=SCL, PB9=SDA | SPI1: PB3/PA6/PA7\r\n");

  printf("UART2: 115200 baud, 8-N-1\r\n\r\n");



  if (tmp102_initialize() != HAL_OK)

  {

    printf("ERROR: TMP102 did not respond on I2C1. Check U7/address/pins.\r\n");

  }

  else

  {

    printf("TMP102 initialized successfully.\r\n");

  }



  if (FRAM_Init() != HAL_OK)

  {

    printf("ERROR: FRAM initialization failed.\r\n");

  }

  else

  {

    uint8_t fram_status = 0U;

    if (FRAM_ReadStatus(&fram_status) != HAL_OK)

      printf("ERROR: Could not communicate with SPI FRAM.\r\n");

    else

      printf("SPI FRAM status register: 0x%02X\r\n", fram_status);

  }



  Logger_LoadMetadata();

  Logger_PrintHelp();

  Logger_PrintStatus();

  /* USER CODE END 2 */



  /* Infinite loop */

  /* USER CODE BEGIN WHILE */

  while (1)

  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    uint8_t received_char;

    if (HAL_UART_Receive(&huart2, &received_char, 1U, LOGGER_UART_TIMEOUT_MS) == HAL_OK)

    {

      if (received_char == '\r' || received_char == '\n')

      {

        if (logger_command_length > 0U)

        {

          logger_command_buffer[logger_command_length] = '\0';

          printf("\r\n");

          Logger_ProcessCommand(logger_command_buffer);

          logger_command_length = 0U;

        }

      }

      else if (received_char == 8U || received_char == 127U)

      {

        if (logger_command_length > 0U)

        {

          logger_command_length--;

          printf("\b \b");

        }

      }

      else if (isprint((int)received_char) && logger_command_length < (sizeof(logger_command_buffer) - 1U))

      {

        logger_command_buffer[logger_command_length++] = (char)received_char;

        printf("%c", received_char);

      }

    }



    if (logger_is_running && ((HAL_GetTick() - logger_last_sample_tick) >= 1000U))

    {

      logger_last_sample_tick = HAL_GetTick();

      Logger_TakeSample();

    }

    /* USER CODE END 3 */

  }

}



/**

  * @brief System Clock Configuration

  * @retval None

  */

void SystemClock_Config(void)

{

  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};



  /** Configure the main internal regulator output voltage

  */

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);



  /** Initializes the RCC Oscillators according to the specified parameters

  * in the RCC_OscInitTypeDef structure.

  */

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;

  RCC_OscInitStruct.HSIState = RCC_HSI_ON;

  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;

  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;

  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;

  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;

  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;

  RCC_OscInitStruct.PLL.PLLN = 8;

  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;

  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;

  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV4;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)

  {

    Error_Handler();

  }



  /** Initializes the CPU, AHB and APB buses clocks

  */

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                              |RCC_CLOCKTYPE_PCLK1;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;

  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;



  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)

  {

    Error_Handler();

  }

}



/**

  * @brief I2C1 Initialization Function

  * @param None

  * @retval None

  */

static void MX_I2C1_Init(void)

{

  hi2c1.Instance = I2C1;

  hi2c1.Init.Timing = 0x00B07CB4;

  hi2c1.Init.OwnAddress1 = 0;

  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;

  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;

  hi2c1.Init.OwnAddress2 = 0;

  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;

  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;

  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

  if (HAL_I2C_Init(&hi2c1) != HAL_OK) Error_Handler();

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) Error_Handler();

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) Error_Handler();

}



/**

  * @brief SPI1 Initialization Function

  * @param None

  * @retval None

  */

static void MX_SPI1_Init(void)

{



  /* USER CODE BEGIN SPI1_Init 0 */



  /* USER CODE END SPI1_Init 0 */



  /* USER CODE BEGIN SPI1_Init 1 */



  /* USER CODE END SPI1_Init 1 */

  /* SPI1 parameter configuration*/

  hspi1.Instance = SPI1;

  hspi1.Init.Mode = SPI_MODE_MASTER;

  hspi1.Init.Direction = SPI_DIRECTION_2LINES;

  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;

  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;

  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;

  hspi1.Init.NSS = SPI_NSS_SOFT;

  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;

  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;

  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;

  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;

  hspi1.Init.CRCPolynomial = 7;

  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;

  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;

  if (HAL_SPI_Init(&hspi1) != HAL_OK)

  {

    Error_Handler();

  }

  /* USER CODE BEGIN SPI1_Init 2 */



  /* USER CODE END SPI1_Init 2 */



}



/**

  * @brief USART2 Initialization Function

  * @param None

  * @retval None

  */

static void MX_USART2_UART_Init(void)

{



  /* USER CODE BEGIN USART2_Init 0 */



  /* USER CODE END USART2_Init 0 */



  /* USER CODE BEGIN USART2_Init 1 */



  /* USER CODE END USART2_Init 1 */

  huart2.Instance = USART2;

  huart2.Init.BaudRate = 115200;

  huart2.Init.WordLength = UART_WORDLENGTH_8B;

  huart2.Init.StopBits = UART_STOPBITS_1;

  huart2.Init.Parity = UART_PARITY_NONE;

  huart2.Init.Mode = UART_MODE_TX_RX;

  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;

  huart2.Init.OverSampling = UART_OVERSAMPLING_16;

  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;

  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;

  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

  if (HAL_UART_Init(&huart2) != HAL_OK)

  {

    Error_Handler();

  }

  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)

  {

    Error_Handler();

  }

  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)

  {

    Error_Handler();

  }

  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)

  {

    Error_Handler();

  }

  /* USER CODE BEGIN USART2_Init 2 */



  /* USER CODE END USART2_Init 2 */



}



/**

  * @brief GPIO Initialization Function

  * @param None

  * @retval None

  */

static void MX_GPIO_Init(void)

{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* USER CODE BEGIN MX_GPIO_Init_1 */



  /* USER CODE END MX_GPIO_Init_1 */



  /* GPIO Ports Clock Enable */

  __HAL_RCC_GPIOA_CLK_ENABLE();

  __HAL_RCC_GPIOB_CLK_ENABLE();

  __HAL_RCC_GPIOC_CLK_ENABLE();



  /*Configure GPIO pin Output Level */

  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);



  /*Configure GPIO pin Output Level */

  HAL_GPIO_WritePin(SPI_CS_GPIO_Port, SPI_CS_Pin, GPIO_PIN_SET);



  /*Configure GPIO pin Output Level */

  HAL_GPIO_WritePin(SPI1_WP_GPIO_Port, SPI1_WP_Pin, GPIO_PIN_SET);



  /*Configure GPIO pin : LD4_Pin */

  GPIO_InitStruct.Pin = LD4_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

  GPIO_InitStruct.Pull = GPIO_NOPULL;

  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(LD4_GPIO_Port, &GPIO_InitStruct);



  /*Configure GPIO pin : SPI_CS_Pin */

  GPIO_InitStruct.Pin = SPI_CS_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

  GPIO_InitStruct.Pull = GPIO_NOPULL;

  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(SPI_CS_GPIO_Port, &GPIO_InitStruct);



  /*Configure GPIO pin : SPI1_WP_Pin */

  GPIO_InitStruct.Pin = SPI1_WP_Pin;

  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;

  GPIO_InitStruct.Pull = GPIO_NOPULL;

  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(SPI1_WP_GPIO_Port, &GPIO_InitStruct);



  /* USER CODE BEGIN MX_GPIO_Init_2 */



  /* USER CODE END MX_GPIO_Init_2 */

}



/* USER CODE BEGIN 4 */

static HAL_StatusTypeDef Logger_SaveMetadata(void)
{
  uint8_t metadata[4];
  metadata[0] = (uint8_t)(logger_start_address >> 8);
  metadata[1] = (uint8_t)(logger_start_address & 0xFFU);
  metadata[2] = (uint8_t)(logger_last_address >> 8);
  metadata[3] = (uint8_t)(logger_last_address & 0xFFU);
  return FRAM_Write(LOGGER_META_START_ADDR, metadata, sizeof(metadata));
}

static void Logger_LoadMetadata(void)
{
  uint8_t metadata[4] = {0U, 0U, 0U, 0U};
  if (FRAM_Read(LOGGER_META_START_ADDR, metadata, sizeof(metadata)) != HAL_OK)
  {
    printf("WARNING: FRAM metadata read failed. Logging is unavailable until communication is restored.\r\n");
    logger_start_address = 0U;
    logger_last_address = 0U;
    logger_metadata_valid = 0U;
    return;
  }

  uint16_t saved_start = (uint16_t)(((uint16_t)metadata[0] << 8) | metadata[1]);
  uint16_t saved_last = (uint16_t)(((uint16_t)metadata[2] << 8) | metadata[3]);

  if ((saved_start == 0U) && (saved_last == 0U))
  {
    logger_start_address = 0U;
    logger_last_address = 0U;
    logger_metadata_valid = 1U;
    printf("FRAM is empty. Ready for a new logging session.\r\n");
  }
  else if ((saved_start < LOGGER_DATA_START_ADDR) ||
           (saved_last < saved_start) ||
           (saved_last > (FRAM_SIZE - LOGGER_RECORD_SIZE)) ||
           (((saved_start - LOGGER_DATA_START_ADDR) % LOGGER_RECORD_SIZE) != 0U) ||
           (((saved_last - LOGGER_DATA_START_ADDR) % LOGGER_RECORD_SIZE) != 0U))
  {
    /* Do not silently overwrite data if the saved pointers are corrupt. */
    logger_start_address = 0U;
    logger_last_address = 0U;
    logger_metadata_valid = 0U;
    printf("ERROR: FRAM metadata is invalid. Use CLEAR before logging again.\r\n");
  }
  else
  {
    logger_start_address = saved_start;
    logger_last_address = saved_last;
    logger_metadata_valid = 1U;
    printf("FRAM metadata restored. Logging will append after 0x%04X.\r\n", saved_last);
  }
}

static void Logger_PrintHelp(void)
{
  printf("Commands (type command then press Enter):\r\n");
  printf("  START  - start/restart logging at one sample per second\r\n");
  printf("  STOP   - stop automatic sampling\r\n");
  printf("  TEMP   - display saved temperatures, count, and address range\r\n");
  printf("  DUMP   - alias for TEMP\r\n");
  printf("  NOW    - take and display one immediate reading (not logged)\r\n");
  printf("  STATUS - display logger state and FRAM pointers\r\n");
  printf("  CLEAR  - erase all FRAM contents and reset pointers\r\n");
  printf("  HELP   - display this command list\r\n\r\n");
}

static void Logger_PrintStatus(void)
{
  uint32_t count = 0U;
  if (logger_start_address != 0U)
    count = ((uint32_t)(logger_last_address - logger_start_address) / LOGGER_RECORD_SIZE) + 1U;

  printf("Logger: %s\r\n", logger_is_running ? "RUNNING" : "STOPPED");
  if (count == 0U)
    printf("Stored samples: 0 | First address: none | Last address: none\r\n\r\n");
  else
    printf("Stored samples: %lu | First address: 0x%04X | Last address: 0x%04X\r\n\r\n",
           (unsigned long)count, logger_start_address, logger_last_address);
}

static void Logger_PrintRawTemperature(uint16_t raw_temperature)
{
  uint16_t raw12 = (uint16_t)(raw_temperature >> 4);
  int32_t signed_raw;
  int32_t scaled_1e4;
  int32_t magnitude;

  if ((raw12 & 0x0800U) != 0U)
    signed_raw = (int32_t)raw12 - 4096;
  else
    signed_raw = (int32_t)raw12;

  /* TMP102 resolution is 0.0625 C = 625 units of 0.0001 C. */
  scaled_1e4 = signed_raw * 625;
  magnitude = (scaled_1e4 < 0) ? -scaled_1e4 : scaled_1e4;
  printf("%s%ld.%04ld", (scaled_1e4 < 0) ? "-" : "",
         (long)(magnitude / 10000), (long)(magnitude % 10000));
}

static void Logger_TakeSample(void)
{
  uint16_t raw_temperature = 0U;
  uint8_t record[LOGGER_RECORD_SIZE];
  HAL_StatusTypeDef status = tmp102_read_temperature_raw(&raw_temperature);
  if (status != HAL_OK)
  {
    printf("TMP102 read error (%d). Sample was not logged.\r\n", (int)status);
    logger_last_sample_tick = HAL_GetTick();
    return;
  }

  uint16_t address;
  if (logger_start_address == 0U)
    address = LOGGER_DATA_START_ADDR;
  else if ((uint32_t)logger_last_address + LOGGER_RECORD_SIZE < FRAM_SIZE)
    address = (uint16_t)(logger_last_address + LOGGER_RECORD_SIZE);
  else
  {
    logger_is_running = 0U;
    printf("FRAM is full. Logging stopped safely. Use CLEAR to erase it.\r\n");
    return;
  }

  /* Store the exact two-byte TMP102 temperature register value, big-endian. */
  record[0] = (uint8_t)(raw_temperature >> 8);
  record[1] = (uint8_t)(raw_temperature & 0xFFU);
  status = FRAM_Write(address, record, sizeof(record));
  if (status != HAL_OK)
  {
    logger_is_running = 0U;
    printf("ERROR: FRAM write failed (%d). Logging stopped.\r\n", (int)status);
    return;
  }

  if (logger_start_address == 0U)
    logger_start_address = address;
  logger_last_address = address;
  status = Logger_SaveMetadata();
  if (status != HAL_OK)
  {
    /* Stop rather than risk overwriting the unindexed record on a later run. */
    logger_is_running = 0U;
    printf("ERROR: FRAM data was written, but pointer update failed. Logging stopped.\r\n");
    return;
  }

  logger_last_sample_tick = HAL_GetTick();
  HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin);
  printf("Logged temperature: ");
  Logger_PrintRawTemperature(raw_temperature);
  printf(" C at FRAM address 0x%04X\r\n", address);
}

static void Logger_DumpCurrentSession(void)
{
  uint16_t address;
  uint8_t record[LOGGER_RECORD_SIZE];
  uint32_t count = 0U;

  printf("\r\n--- Saved TMP102 temperature readings ---\r\n");
  if (logger_start_address == 0U)
  {
    printf("No temperature readings are stored.\r\n\r\n");
    return;
  }

  printf("First data address: 0x%04X\r\n", logger_start_address);
  printf("Last data address:  0x%04X\r\n", logger_last_address);
  printf("No. of data points: %lu\r\n\r\n",
         (unsigned long)(((uint32_t)(logger_last_address - logger_start_address) / LOGGER_RECORD_SIZE) + 1U));

  address = logger_start_address;
  while (address <= logger_last_address)
  {
    uint16_t raw_temperature;
    if (FRAM_Read(address, record, sizeof(record)) != HAL_OK)
    {
      printf("FRAM read error at 0x%04X.\r\n", address);
      break;
    }
    raw_temperature = (uint16_t)(((uint16_t)record[0] << 8) | record[1]);
    printf("%4lu) Address 0x%04X: ", (unsigned long)(++count), address);
    Logger_PrintRawTemperature(raw_temperature);
    printf(" C (raw 0x%04X)\r\n", raw_temperature);
    if (address == logger_last_address)
      break;
    address = (uint16_t)(address + LOGGER_RECORD_SIZE);
  }
  printf("--- End of saved readings ---\r\n\r\n");
}

static HAL_StatusTypeDef Logger_ClearMemory(void)
{
  uint8_t zeros[64] = {0U};
  uint16_t address = 0U;
  while (address < FRAM_SIZE)
  {
    uint16_t remaining = (uint16_t)(FRAM_SIZE - address);
    uint16_t count = (remaining > sizeof(zeros)) ? (uint16_t)sizeof(zeros) : remaining;
    HAL_StatusTypeDef status = FRAM_Write(address, zeros, count);
    if (status != HAL_OK)
      return status;
    address = (uint16_t)(address + count);
  }
  return HAL_OK;
}

static void Logger_ProcessCommand(char *command)
{
  for (char *p = command; *p != '\0'; ++p)
    *p = (char)toupper((unsigned char)*p);
  while (*command == ' ' || *command == '\t')
    command++;
  size_t length = strlen(command);
  while (length > 0U && (command[length - 1U] == ' ' || command[length - 1U] == '\t'))
    command[--length] = '\0';

  if (length == 0U)
    return;

  if (strcmp(command, "START") == 0)
  {
    if (!logger_metadata_valid)
    {
      printf("FRAM metadata is invalid or unreadable. Run CLEAR before START.\r\n\r\n");
      return;
    }
    if (logger_is_running)
    {
      printf("Logger is already running.\r\n\r\n");
      return;
    }
    if ((logger_start_address != 0U) &&
        ((uint32_t)logger_last_address + LOGGER_RECORD_SIZE >= FRAM_SIZE))
    {
      printf("FRAM is full. Run CLEAR before starting another log.\r\n\r\n");
      return;
    }
    /* Existing pointers are retained, so START resumes by appending after reset. */
    logger_last_sample_tick = HAL_GetTick();
    logger_is_running = 1U;
    printf("Logging started. First sample will be taken in one second.\r\n\r\n");
  }
  else if (strcmp(command, "STOP") == 0)
  {
    logger_is_running = 0U;
    printf("Logging stopped. Saved FRAM data and pointers were retained.\r\n\r\n");
  }
  else if ((strcmp(command, "TEMP") == 0) || (strcmp(command, "DUMP") == 0))
  {
    Logger_DumpCurrentSession();
  }
  else if (strcmp(command, "NOW") == 0)
  {
    uint16_t raw_temperature = 0U;
    HAL_StatusTypeDef status = tmp102_read_temperature_raw(&raw_temperature);
    if (status == HAL_OK)
    {
      printf("Current temperature: ");
      Logger_PrintRawTemperature(raw_temperature);
      printf(" C (not logged)\r\n\r\n");
    }
    else
      printf("TMP102 read failed (%d).\r\n\r\n", (int)status);
  }
  else if (strcmp(command, "STATUS") == 0)
  {
    Logger_PrintStatus();
  }
  else if (strcmp(command, "CLEAR") == 0)
  {
    logger_is_running = 0U;
    printf("Clearing all 8192 FRAM bytes...\r\n");
    if (Logger_ClearMemory() == HAL_OK)
    {
      logger_start_address = 0U;
      logger_last_address = 0U;
      logger_metadata_valid = 1U;
      printf("FRAM erased. Metadata pointers reset.\r\n\r\n");
    }
    else
    {
      printf("ERROR: FRAM erase failed. Check SPI/FRAM wiring.\r\n\r\n");
    }
  }
  else if ((strcmp(command, "HELP") == 0) || (strcmp(command, "?") == 0))
  {
    Logger_PrintHelp();
  }
  else
  {
    printf("Unknown command: %s. Type HELP for commands.\r\n\r\n", command);
  }
}

/* USER CODE END 4 */



/**

  * @brief  This function is executed in case of error occurrence.

  * @retval None

  */

void Error_Handler(void)

{

  /* USER CODE BEGIN Error_Handler_Debug */



  __disable_irq();



  while (1)



  {



  }



  /* USER CODE END Error_Handler_Debug */

}

#ifdef USE_FULL_ASSERT

/**

  * @brief  Reports the name of the source file and the source line number

  *         where the assert_param error has occurred.

  * @param  file: pointer to the source file name

  * @param  line: assert_param error line source number

  * @retval None

  */

void assert_failed(uint8_t *file, uint32_t line)

{

  /* USER CODE BEGIN 6 */

  /* User can add his own implementation to report the file name and line number,

     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* USER CODE END 6 */

}

#endif /* USE_FULL_ASSERT */
