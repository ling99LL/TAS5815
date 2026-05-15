/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tas5815.h
  * @brief   Header file for TAS5815 driver functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Your Company/Name.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TAS5815_H
#define __TAS5815_H

/* Includes ------------------------------------------------------------------*/
#include "main.h" /* HAL library and necessary definitions */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/* TAS5815 I2C Address (7-bit: 0x54, 8-bit write: 0xA8) */
#define TAS5815_I2C_ADDR_7BIT   0x54

/* Exported functions --------------------------------------------------------*/
void delay_ms(uint32_t ms);
void tas5815_Initialize(void);
void set_tas5815_volume(uint32_t count);
void tas5815_mute(void);
void tas5815_unmute(void);
void tas5815_set_analog_gain(uint8_t gain_index);
void tas5815_set_modulation(uint8_t modulation);
void tas5815_set_fsw(uint8_t fsw_sel);
void tas5815_sleep(void);
void tas5815_wakeup(void);

#endif /* __TAS5815_H */

/************************ (C) COPYRIGHT Your Company *****END OF FILE****/
