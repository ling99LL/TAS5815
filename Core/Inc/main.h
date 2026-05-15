/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
/* 音量控制函数（供菜单系统同步硬件计数器使用） */
int32_t GetVolume(void);
void SetVolume(int32_t vol);
void SetModulation(uint8_t modulation);
void SetFSW(uint8_t fsw);
void SetVoltage(uint8_t voltage);
void RestoreVoltage(void);
/* 关机/唤醒函数 */
void System_PowerOff(void);
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
/* 编码器按键 - PB9 */
#define ENC_KEY_Pin GPIO_PIN_9
#define ENC_KEY_GPIO_Port GPIOB

/* 蓝牙控制引脚 - QCC3034 */
#define BT_PAIR_Pin GPIO_PIN_12      /* PB12 - 暂停/蓝牙配对 */
#define BT_PAIR_GPIO_Port GPIOB
#define BT_VOL_DOWN_Pin GPIO_PIN_13  /* PB13 - VOL-/上一曲 */
#define BT_VOL_DOWN_GPIO_Port GPIOB
#define BT_VOL_UP_Pin GPIO_PIN_14    /* PB14 - VOL+/下一曲 */
#define BT_VOL_UP_GPIO_Port GPIOB
#define BT_USB_DET_Pin GPIO_PIN_0    /* PB0 - 蓝牙USB口检测 */
#define BT_USB_DET_GPIO_Port GPIOB
#define BT_PLAY_DET_Pin GPIO_PIN_6   /* PA6 - 播放检测 */
#define BT_PLAY_DET_GPIO_Port GPIOA
#define BT_LED0_Pin GPIO_PIN_15      /* PB15 - LED0 */
#define BT_LED0_GPIO_Port GPIOB
#define BT_LED1_Pin GPIO_PIN_4       /* PA4 - LED1 */
#define BT_LED1_GPIO_Port GPIOA
#define BT_EN_Pin GPIO_PIN_10        /* PA10 - 蓝牙模块启动，高电平有效 */
#define BT_EN_GPIO_Port GPIOA

/* PD诱骗控制引脚 - CH224K */
#define PD_5V_Pin GPIO_PIN_9         /* PA9 - 5V控制，高电平有效 */
#define PD_5V_GPIO_Port GPIOA
#define PD_20V_Pin GPIO_PIN_10       /* PB10 - 20V控制，高电平有效 */
#define PD_20V_GPIO_Port GPIOB
#define PD_VOLT_DET_Pin GPIO_PIN_1   /* PB1 - 电压检测ADC (分压比: 51k/10k) */
#define PD_VOLT_DET_GPIO_Port GPIOB

/* 12V是默认状态：PD_5V=0, PD_20V=0 */
/* 电压检测计算: 实际电压 = ADC值 * 3.3V / 4095 * 6.1 */
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
