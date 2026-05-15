/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    at24c02.h
  * @brief   AT24C02 EEPROM驱动头文件
  ******************************************************************************
  * @attention
  * 
  * 功能说明：
  * - I2C接口EEPROM，容量2Kbit (256字节)
  * - 设备地址：0x50 (7-bit)
  * - 页写入大小：8字节
  * - 用于存储音量等配置参数
  * 
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __AT24C02_H
#define __AT24C02_H

#include "main.h"
#include <stdint.h>

/* AT24C02 设备地址 (7-bit) */
#define AT24C02_ADDR        0x50

/* 音量存储地址 */
#define AT24C02_VOLUME_ADDR 0x00

/* 音量有效标志地址 */
#define AT24C02_FLAG_ADDR   0x01

/* 音量有效标志值 */
#define AT24C02_VALID_FLAG  0xA5

/* 电压设置存储地址 */
#define AT24C02_VOLTAGE_ADDR        0x02
#define AT24C02_VOLTAGE_FLAG_ADDR   0x03
#define AT24C02_VOLTAGE_VALID_FLAG  0x5A

/* 调制模式存储地址 */
#define AT24C02_MODULATION_ADDR        0x04
#define AT24C02_MODULATION_FLAG_ADDR   0x05
#define AT24C02_MODULATION_VALID_FLAG  0x3C

/* 开关频率存储地址 */
#define AT24C02_FSW_ADDR        0x06
#define AT24C02_FSW_FLAG_ADDR   0x07
#define AT24C02_FSW_VALID_FLAG  0xC3

/* 关机设置存储地址 */
#define AT24C02_POWEROFF_ADDR        0x08
#define AT24C02_POWEROFF_FLAG_ADDR   0x09
#define AT24C02_POWEROFF_VALID_FLAG  0xD7

/* AMP增益存储地址 */
#define AT24C02_AMP_ADDR        0x0A
#define AT24C02_AMP_FLAG_ADDR   0x0B
#define AT24C02_AMP_VALID_FLAG  0xE1

/**
 * @brief 初始化 AT24C02
 * @retval 0: 成功，1: 失败
 */
uint8_t AT24C02_Init(void);

/**
 * @brief 写入一个字节
 * @param addr: EEPROM内部地址 (0-255)
 * @param data: 要写入的数据
 * @retval 0: 成功, 1: 失败
 */
uint8_t AT24C02_WriteByte(uint8_t addr, uint8_t data);

/**
 * @brief 读取一个字节
 * @param addr: EEPROM内部地址 (0-255)
 * @param data: 读取的数据指针
 * @retval 0: 成功, 1: 失败
 */
uint8_t AT24C02_ReadByte(uint8_t addr, uint8_t *data);

/**
 * @brief 保存音量到EEPROM
 * @param volume: 音量值 (0-100)
 * @retval 0: 成功, 1: 失败
 */
uint8_t AT24C02_SaveVolume(uint8_t volume);

/**
 * @brief 从EEPROM读取音量
 * @param volume: 音量值指针
 * @retval 0: 成功读取有效值, 1: 无有效值或失败
 */
uint8_t AT24C02_LoadVolume(uint8_t *volume);

/**
 * @brief 保存电压设置到 EEPROM
 * @param voltage: 电压设置值 (0=5V, 1=12V, 2=20V)
 * @retval 0: 成功，1: 失败
 */
uint8_t AT24C02_SaveVoltage(uint8_t voltage);

/**
 * @brief 从 EEPROM 读取电压设置
 * @param voltage: 电压设置值指针
 * @retval 0: 成功读取有效值，1: 无有效值或失败
 */
uint8_t AT24C02_LoadVoltage(uint8_t *voltage);

uint8_t AT24C02_SaveModulation(uint8_t modulation);
uint8_t AT24C02_LoadModulation(uint8_t *modulation);

uint8_t AT24C02_SaveFSW(uint8_t fsw);
uint8_t AT24C02_LoadFSW(uint8_t *fsw);

uint8_t AT24C02_SavePowerOff(uint8_t mode);
uint8_t AT24C02_LoadPowerOff(uint8_t *mode);

uint8_t AT24C02_SaveAMP(uint8_t gain);
uint8_t AT24C02_LoadAMP(uint8_t *gain);

#endif /* __AT24C02_H */
