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
#include "adc.h"
#include "i2c.h"
#include "i2s.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "oled.h"
#include "stdio.h"
#include "tas5815.h"
#include "menu.h"
#include "at24c02.h"
#include "adc.h"
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

/* USER CODE BEGIN PV */
/* 音量保存相关参数 */
#define VOLUME_SAVE_DELAY  2000  /* 音量稳定2秒后保存 (ms) */

static uint32_t g_volumeChangeTime = 0;  /* 音量变化时间戳 */
static uint8_t  g_volumeNeedSave = 0;    /* 音量需要保存标志 */
static int32_t  g_volumeToSave = 0;      /* 待保存的音量值 */

/* 音量变量（供菜单系统同步硬件计数器使用） */
static int32_t g_volume = 30; /* 默认音量30 */

/* 音量显示消息缓冲 */
char volStr[4] = "99";

/* 音量最大值 */
#define VOLUME_MAX  99

/* 播放状态检测阈值：ADC值 > 1240 (约1V) 认为正在播放 */
#define PLAY_THRESHOLD_ADC  1240
#define PLAY_SAMPLE_COUNT   5

/* 关机模式定义 */
#define POWEROFF_MODE_10MIN      0  /* 无播放10分钟关机 */
#define POWEROFF_MODE_30MIN     1  /* 无播放30分钟关机 */
#define POWEROFF_MODE_FOLLOW    2  /* 随主机关机 */
#define POWEROFF_MODE_DISABLED  3  /* 不自动关机 */

/* 自动关机时间：默认30分钟 */
#define AUTO_POWER_OFF_MS_DEFAULT   (30 * 60 * 1000)
#define AUTO_POWER_OFF_MS_10MIN      (10 * 60 * 1000)

static uint8_t g_lastPlayState = 0;
static uint8_t g_currentModulation = 0;
static uint8_t g_currentFSW = 0;
static uint8_t g_currentVoltage = 1;
static uint32_t g_lastPlayTime = 0;

/* 关机设置：0=10分钟, 1=30分钟, 2=随主机, 3=不自动关机 */
uint8_t g_powerOffMode = POWEROFF_MODE_30MIN;
static uint8_t g_pb0LastState = 0;

void SetPowerOffMode(uint8_t mode)
{
    g_powerOffMode = mode;
}
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* 引用I2C句柄 */
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c3; /* OLED和AT24C02使用I2C3 */

/* 获取当前音量 */
int32_t GetVolume(void)
{
    return g_volume;
}

/* 设置音量（同时更新硬件计数器） */
void SetVolume(int32_t vol)
{
    if (vol < 0) vol = 0;
    if (vol > VOLUME_MAX) vol = VOLUME_MAX;
    g_volume = vol;
    __HAL_TIM_SET_COUNTER(&htim2, (uint32_t)(vol << 1));
}

void SetModulation(uint8_t modulation)
{
    g_currentModulation = modulation;
    tas5815_set_modulation(modulation);
}

void SetFSW(uint8_t fsw)
{
    g_currentFSW = fsw;
    tas5815_set_fsw(fsw);
}

void SetVoltage(uint8_t voltage)
{
    g_currentVoltage = voltage;
    switch(voltage) {
        case 0:
            HAL_GPIO_WritePin(PD_5V_GPIO_Port, PD_5V_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(PD_20V_GPIO_Port, PD_20V_Pin, GPIO_PIN_RESET);
            break;
        case 1:
            HAL_GPIO_WritePin(PD_5V_GPIO_Port, PD_5V_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(PD_20V_GPIO_Port, PD_20V_Pin, GPIO_PIN_RESET);
            break;
        case 2:
            HAL_GPIO_WritePin(PD_5V_GPIO_Port, PD_5V_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(PD_20V_GPIO_Port, PD_20V_Pin, GPIO_PIN_SET);
            break;
    }
}

void RestoreVoltage(void)
{
    SetVoltage(g_currentVoltage);
}

static uint32_t Read_ADC_Channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = channel;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t value = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return value;
}

void System_PowerOff(void)
{
    if (g_volumeNeedSave) {
        AT24C02_SaveVolume((uint8_t)g_volumeToSave);
        g_volumeNeedSave = 0;
    }
    
    HAL_GPIO_WritePin(PD_5V_GPIO_Port, PD_5V_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(PD_20V_GPIO_Port, PD_20V_Pin, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(BT_EN_GPIO_Port, BT_EN_Pin, GPIO_PIN_RESET);
    tas5815_sleep();
    HAL_Delay(50);
    OLED_DisPlay_Off();

    HAL_SuspendTick();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
    HAL_NVIC_DisableIRQ(EXTI0_IRQn);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_9);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);

    SystemClock_Config();
    HAL_ResumeTick();

    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_Delay(100);

    RestoreVoltage();
    OLED_NewFrame();
    OLED_ShowFrame();
    OLED_DisPlay_On();
    HAL_Delay(500);
    HAL_GPIO_WritePin(BT_EN_GPIO_Port, BT_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(200);
    tas5815_wakeup();
    set_tas5815_volume((uint32_t)g_volume);
    g_lastPlayTime = HAL_GetTick();
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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_I2S1_Init();
  MX_SPI3_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  /* USER CODE BEGIN 2 */

  HAL_Delay(500);
  /* 默认开启蓝牙模块 (PA10 高电平) */
  HAL_GPIO_WritePin(BT_EN_GPIO_Port, BT_EN_Pin, GPIO_PIN_SET);

  /* 使能PWR时钟并解锁备份域访问（LSE配置需要） */
  __HAL_RCC_PWR_CLK_ENABLE();
  HAL_PWR_EnableBkUpAccess();

  /* 开启外部低速晶振 LSE (32.768kHz)，带超时保护 */
  __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);
  uint32_t lseTimeout = 0;
  while(__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {
      if (++lseTimeout > 50000) break;  /* 约500ms超时 */
  }

  HAL_Delay(200);
  OLED_Init();
  OLED_NewFrame();
  OLED_PrintString(4, 2, "星火计划", &font30x30, OLED_COLOR_NORMAL);
  OLED_PrintString(4, 34, "鸢枭出品", &font30x30, OLED_COLOR_NORMAL);
  OLED_ShowFrame();
  MX_I2S1_Init();
  MX_SPI3_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
/* 启动编码器 */
  HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
  
  /* EEPROM 初始化 */
  if (AT24C02_Init() == 0) {
      /* 尝试从 EEPROM 读取记忆音量 */
      uint8_t savedVolume = 30;  /* 默认音量 30 */
      if (AT24C02_LoadVolume(&savedVolume) == 0) {
          g_volume = savedVolume;
      }
      
      /* 尝试加载记忆的电压设置并立即应用 */
      uint8_t savedVoltage = 1;  /* 默认 12V */
      if (AT24C02_LoadVoltage(&savedVoltage) == 0) {
          SetVoltage(savedVoltage);
      }
  }
  
  HAL_Delay(500);  /* 显示启动信息 500ms */
  
  /* 设置编码器初始值（音量左移 1 位） */
  __HAL_TIM_SET_COUNTER(&htim2, (uint32_t)(g_volume << 1));
  HAL_Delay(2);
  
   /* 初始化 TAS5815 音频放大器 */
   tas5815_Initialize();
   
   /* 加载记忆的调制模式并配置 */
uint8_t savedModulation = 0;
   if (AT24C02_LoadModulation(&savedModulation) == 0) {
       g_currentModulation = savedModulation;
       tas5815_set_modulation(savedModulation);
   }
   
    uint8_t savedFSW = 0;
   if (AT24C02_LoadFSW(&savedFSW) == 0) {
       g_currentFSW = savedFSW;
       tas5815_set_fsw(savedFSW);
   }
   
   uint8_t savedPowerOff = POWEROFF_MODE_30MIN;
   if (AT24C02_LoadPowerOff(&savedPowerOff) == 0) {
       g_powerOffMode = savedPowerOff;
   }
   
   uint8_t savedAMP = 5;
   if (AT24C02_LoadAMP(&savedAMP) == 0) {
       tas5815_set_analog_gain(savedAMP);
   }
   
   g_pb0LastState = HAL_GPIO_ReadPin(BT_USB_DET_GPIO_Port, BT_USB_DET_Pin);
    
     /* 初始化菜单系统（不再设置默认电压） */
     Menu_Init();
    
    g_lastPlayTime = HAL_GetTick();
   /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  int32_t last_count = -1; /* 检测音量变化 */
  while (1)
  {
      /* 处理菜单系统（包含编码器和按键处理） */
      Menu_Process();
      
      /* 检查是否需要关机 */
      if (Menu_IsPowerOffPending()) {
          Menu_ClearPowerOffPending();
          System_PowerOff();
      }
      
      /* 仅在音量界面时执行音量调节逻辑 */
      if (Menu_GetState() == MENU_STATE_VOLUME && !Menu_IsInPowerOffMode()) {
          /* 清空OLED显示缓冲 */
          OLED_NewFrame();

          /* 1. 获取硬件计数器的当前值 */
          uint32_t raw_count = __HAL_TIM_GET_COUNTER(&htim2); 

          /* 2. 计算音量值（右移1位，每2个编码器脉冲变化1个音量单位） */
          /* 顺时针旋转增加音量 */
          int32_t current_count = ((int32_t)raw_count) >> 1;

          /* 3. 边界检查与硬件同步 (解决"死区"问题的关键) */
          if (current_count < 0) {
              g_volume = 0;
              __HAL_TIM_SET_COUNTER(&htim2, 0); 
          } 
          else if (current_count > VOLUME_MAX) {
              g_volume = VOLUME_MAX;
              __HAL_TIM_SET_COUNTER(&htim2, VOLUME_MAX << 1); 
          } 
          else {
              g_volume = current_count;
          }

          /* 检测音量变化并更新硬件 */
          if (g_volume != last_count) {
              
              /* 设置TAS5815音量 */
              set_tas5815_volume((uint32_t)g_volume); 
              
              /* 标记需要保存，记录时间和待保存值 */
              g_volumeChangeTime = HAL_GetTick();
              g_volumeToSave = g_volume;
              g_volumeNeedSave = 1;
              
              last_count = g_volume;
          }
          
          /* 检查是否需要保存音量到EEPROM（音量稳定2秒后保存） */
           if (g_volumeNeedSave && (HAL_GetTick() - g_volumeChangeTime >= VOLUME_SAVE_DELAY)) {
               AT24C02_SaveVolume((uint8_t)g_volumeToSave);
               g_volumeNeedSave = 0;
           }

/* VOLUME标题 + 音量数字 + 进度条 (右侧对齐) */
           sprintf(volStr, "%02ld", (long)g_volume);
           uint8_t volX = 128 - 2 * 30;
           OLED_PrintASCIIString(volX, 5, "VOLUME", &afont16x8, OLED_COLOR_NORMAL);
            OLED_PrintString(volX - 8, 20, volStr, &fontNum30x30, OLED_COLOR_NORMAL);

           /* 音量进度条 */
           uint8_t barY = 55;
           uint8_t barH = 10;
           uint8_t barW = 128;
           uint32_t drawWidth = (uint32_t)g_volume * barW / VOLUME_MAX;
           OLED_DrawRectangle(0, barY, barW, barH, OLED_COLOR_NORMAL);
           if (drawWidth > 0) {
               OLED_DrawFilledRectangle(1, barY + 1, (uint8_t)drawWidth, barH - 2, OLED_COLOR_NORMAL);
           }

           /* 左侧状态显示 (x=4, font10x10高度10像素, 间隔2像素) */
           uint8_t leftX = 4;
           uint8_t leftY = 4;
           char statusBuf[16];

              /* 第1行: 播放状态 */
              static uint32_t lastStartTime = 0;
              for (int i = 0; i < 10; i++) {
                  if (Read_ADC_Channel(ADC_CHANNEL_6) > PLAY_THRESHOLD_ADC) {
                      lastStartTime = HAL_GetTick();
                      break;
                  }
              }
              g_lastPlayState = ((HAL_GetTick() - lastStartTime) < 100) ? 1 : 0;
             if (g_lastPlayState) {
                 g_lastPlayTime = HAL_GetTick();
             }
             OLED_PrintString(leftX, leftY, g_lastPlayState ? "START" : "STOP ", &font10x10, OLED_COLOR_NORMAL);
             
             uint8_t pb0CurrentState = HAL_GPIO_ReadPin(BT_USB_DET_GPIO_Port, BT_USB_DET_Pin);
             uint8_t autoPowerOff = 0;
             
             if (g_powerOffMode == POWEROFF_MODE_FOLLOW) {
                 if (g_pb0LastState == GPIO_PIN_SET && pb0CurrentState == GPIO_PIN_RESET) {
                     autoPowerOff = 1;
                 } else if (pb0CurrentState == GPIO_PIN_RESET && !g_lastPlayState) {
                     uint32_t timeout = AUTO_POWER_OFF_MS_10MIN;
                     if ((HAL_GetTick() - g_lastPlayTime >= timeout)) {
                         autoPowerOff = 1;
                     }
                 }
             } else if (g_powerOffMode != POWEROFF_MODE_DISABLED) {
                 uint32_t timeout = (g_powerOffMode == POWEROFF_MODE_10MIN) ? 
                                    AUTO_POWER_OFF_MS_10MIN : AUTO_POWER_OFF_MS_DEFAULT;
                 if (!g_lastPlayState && (HAL_GetTick() - g_lastPlayTime >= timeout)) {
                     autoPowerOff = 1;
                 }
             }
             
             g_pb0LastState = pb0CurrentState;
             
             if (autoPowerOff) {
                 System_PowerOff();
             }

           /* 第2行: 电压 */
           leftY += 12;
           uint32_t adcVolt = Read_ADC_Channel(ADC_CHANNEL_9);
           float voltage = (float)adcVolt * 3.3f * 6.1f / 4095.0f;
           sprintf(statusBuf, "%dV", (int)(voltage + 0.5f));
           OLED_PrintString(leftX, leftY, statusBuf, &font10x10, OLED_COLOR_NORMAL);

           /* 第3行: 调制模式 */
           leftY += 12;
           switch(g_currentModulation) {
               case 0: OLED_PrintString(leftX, leftY, "BD", &font10x10, OLED_COLOR_NORMAL); break;
               case 1: OLED_PrintString(leftX, leftY, "1SPW", &font10x10, OLED_COLOR_NORMAL); break;
               case 3: OLED_PrintString(leftX, leftY, "BD/1SPW", &font10x10, OLED_COLOR_NORMAL); break;
               default: OLED_PrintString(leftX, leftY, "?", &font10x10, OLED_COLOR_NORMAL); break;
           }

           /* 第4行: 开关频率 */
           leftY += 12;
           switch(g_currentFSW) {
               case 0: OLED_PrintString(leftX, leftY, "768K", &font10x10, OLED_COLOR_NORMAL); break;
               case 1: OLED_PrintString(leftX, leftY, "384K", &font10x10, OLED_COLOR_NORMAL); break;
               case 5: OLED_PrintString(leftX, leftY, "1024K", &font10x10, OLED_COLOR_NORMAL); break;
               default: OLED_PrintString(leftX, leftY, "?", &font10x10, OLED_COLOR_NORMAL); break;
           }

           /* 刷新屏幕 */
           OLED_ShowFrame();
      }
      
      /* 延时以稳定显示 */
      HAL_Delay(3); 
      
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 5;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

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
