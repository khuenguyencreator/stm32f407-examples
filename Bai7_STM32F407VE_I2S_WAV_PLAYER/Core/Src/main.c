/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "fatfs.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CS43L22_ADDR      0x94      /* dia chi I2C (8 bit) cua CS43L22 */
#define AUDIO_BUF_SIZE    4096      /* so mau 16 bit trong buffer DMA */
#define WAV_FILE_NAME     "music.wav"

#define PLAY_IDLE         0
#define PLAY_RUNNING      1
#define PLAY_DONE         2
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_tx;

/* USER CODE BEGIN PV */
extern ApplicationTypeDef Appli_state;

int16_t audio_buf[AUDIO_BUF_SIZE];
volatile uint8_t half_done = 0;     /* DMA da phat xong nua dau buffer */
volatile uint8_t full_done = 0;     /* DMA da phat xong nua sau buffer */
uint8_t play_state = PLAY_IDLE;
uint32_t data_remain = 0;           /* so byte am thanh con lai trong file */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* Ghi 1 thanh ghi cua CS43L22 qua I2C */
static void CS43L22_Write(uint8_t reg, uint8_t value)
{
  uint8_t data[2] = {reg, value};
  HAL_I2C_Master_Transmit(&hi2c1, CS43L22_ADDR, data, 2, 100);
}

/* Dat am luong 0 - 100 (%) */
static void CS43L22_SetVolume(uint8_t volume)
{
  uint8_t vol = (uint8_t)((volume * 255) / 100);

  /* Thanh ghi Master Volume: 0x19 = -102dB ... 0xFF = -0.5dB, 0x00 = 0dB */
  if (vol > 0xE6)
  {
    vol = vol - 0xE7;
  }
  else
  {
    vol = vol + 0x19;
  }
  CS43L22_Write(0x20, vol);   /* Master Volume kenh A */
  CS43L22_Write(0x21, vol);   /* Master Volume kenh B */
}

static void CS43L22_Init(uint8_t volume)
{
  /* Keo chan RESET (PD4) len 1 de CS43L22 hoat dong */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_Delay(10);

  CS43L22_Write(0x02, 0x01);  /* Power Ctl 1: tat codec trong luc cau hinh */
  CS43L22_Write(0x04, 0xAF);  /* Power Ctl 2: bat tai nghe, tat loa */
  CS43L22_Write(0x05, 0x81);  /* Clocking: tu nhan dien toc do MCLK */
  CS43L22_Write(0x06, 0x04);  /* Interface: slave, chuan I2S Philips, 16 bit */
  CS43L22_SetVolume(volume);
  CS43L22_Write(0x0A, 0x00);  /* Analog ZC and SR: tat */
  CS43L22_Write(0x27, 0x00);  /* Limiter: tat */
  CS43L22_Write(0x1F, 0x0F);  /* Tone: bass, treble mac dinh */
  CS43L22_Write(0x1A, 0x0A);  /* PCM volume kenh A */
  CS43L22_Write(0x1B, 0x0A);  /* PCM volume kenh B */
  CS43L22_Write(0x0E, 0x06);  /* Misc Ctl: bat digital soft ramp */
  CS43L22_Write(0x02, 0x9E);  /* Power Ctl 1: bat codec */
}

static uint32_t Read_LE32(const uint8_t *p)
{
  return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

/* Mo file WAV, kiem tra dinh dang, dua con tro file toi dau du lieu am thanh */
static uint8_t WAV_Open(const char *name)
{
  uint8_t header[12];
  uint8_t chunk[8];
  uint8_t fmt[16];
  uint32_t size;
  UINT br;

  if (f_open(&USBHFile, name, FA_READ) != FR_OK)
  {
    return 0;
  }
  f_read(&USBHFile, header, 12, &br);
  if (memcmp(header, "RIFF", 4) != 0 || memcmp(&header[8], "WAVE", 4) != 0)
  {
    f_close(&USBHFile);
    return 0;
  }

  /* Duyet qua cac chunk cho toi khi gap chunk "data" */
  while (1)
  {
    if (f_read(&USBHFile, chunk, 8, &br) != FR_OK || br < 8)
    {
      f_close(&USBHFile);
      return 0;
    }
    size = Read_LE32(&chunk[4]);

    if (memcmp(chunk, "fmt ", 4) == 0)
    {
      f_read(&USBHFile, fmt, 16, &br);
      /* Chi ho tro WAV 2 kenh, 44100Hz, 16 bit */
      if (fmt[2] != 2 || Read_LE32(&fmt[4]) != 44100 || fmt[14] != 16)
      {
        f_close(&USBHFile);
        return 0;
      }
      f_lseek(&USBHFile, f_tell(&USBHFile) + size - 16);
    }
    else if (memcmp(chunk, "data", 4) == 0)
    {
      data_remain = size;
      return 1;
    }
    else
    {
      f_lseek(&USBHFile, f_tell(&USBHFile) + size);   /* bo qua chunk khac */
    }
  }
}

/* Doc tiep du lieu tu file vao buffer, het file thi dien 0 (im lang) */
static void WAV_Fill(int16_t *dst, uint32_t bytes)
{
  UINT br = 0;
  uint32_t n = (data_remain < bytes) ? data_remain : bytes;

  if (n > 0)
  {
    f_read(&USBHFile, dst, n, &br);
  }
  data_remain -= br;
  if (br < bytes)
  {
    memset((uint8_t *)dst + br, 0, bytes - br);
  }
}
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
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_I2S3_Init();
  MX_FATFS_Init();
  MX_USB_HOST_Init();
  /* USER CODE BEGIN 2 */
  GPIO_InitTypeDef gpio = {0};

  /* PD4 noi voi chan RESET cua CS43L22 */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  gpio.Pin = GPIO_PIN_4;
  gpio.Mode = GPIO_MODE_OUTPUT_PP;
  gpio.Pull = GPIO_NOPULL;
  gpio.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &gpio);

  CS43L22_Init(70);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
    /* USB da san sang: mount, mo file WAV va bat dau phat */
    if (Appli_state == APPLICATION_READY && play_state == PLAY_IDLE)
    {
      play_state = PLAY_DONE;
      if (f_mount(&USBHFatFS, USBHPath, 1) == FR_OK && WAV_Open(WAV_FILE_NAME))
      {
        WAV_Fill(audio_buf, sizeof(audio_buf));
        HAL_I2S_Transmit_DMA(&hi2s3, (uint16_t *)audio_buf, AUDIO_BUF_SIZE);
        play_state = PLAY_RUNNING;
      }
    }

    if (play_state == PLAY_RUNNING)
    {
      /* DMA dang phat nua nay thi nap du lieu moi vao nua kia */
      if (half_done)
      {
        half_done = 0;
        WAV_Fill(&audio_buf[0], sizeof(audio_buf) / 2);
      }
      if (full_done)
      {
        full_done = 0;
        WAV_Fill(&audio_buf[AUDIO_BUF_SIZE / 2], sizeof(audio_buf) / 2);
      }
      if (data_remain == 0)
      {
        HAL_I2S_DMAStop(&hi2s3);
        f_close(&USBHFile);
        play_state = PLAY_DONE;
      }
    }

    /* Rut USB ra: dung phat, cho cam lai */
    if (Appli_state == APPLICATION_DISCONNECT)
    {
      if (play_state == PLAY_RUNNING)
      {
        HAL_I2S_DMAStop(&hi2s3);
      }
      f_mount(NULL, USBHPath, 0);
      play_state = PLAY_IDLE;
      Appli_state = APPLICATION_IDLE;
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 271;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
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

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_44K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}

/* USER CODE BEGIN 4 */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance == SPI3)
  {
    half_done = 1;
  }
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  if (hi2s->Instance == SPI3)
  {
    full_done = 1;
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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
