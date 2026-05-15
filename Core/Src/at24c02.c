/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    at24c02.c
  * @brief   AT24C02 EEPROM驱动实现
  ******************************************************************************
  * @attention
  * 
  * 使用I2C3接口（与OLED共用）
  * 设备地址：0x50
  * 
  ******************************************************************************
  */
/* USER CODE END Header */

#include "at24c02.h"
#include "i2c.h"

/* EEPROM写入等待时间 (ms) */
#define AT24C02_WRITE_DELAY  5

/**
 * @brief 初始化AT24C02
 * @retval 0: 成功, 1: 失败
 */
uint8_t AT24C02_Init(void)
{
    /* 检测设备是否响应 */
    return (HAL_I2C_IsDeviceReady(&hi2c3, (AT24C02_ADDR << 1), 3, 100) != HAL_OK);
}

/**
 * @brief 写入一个字节
 * @param addr: EEPROM内部地址 (0-255)
 * @param data: 要写入的数据
 * @retval 0: 成功, 1: 失败
 */
uint8_t AT24C02_WriteByte(uint8_t addr, uint8_t data)
{
    HAL_StatusTypeDef status;
    
    /* 发送地址和数据 */
    status = HAL_I2C_Mem_Write(&hi2c3, 
                               (AT24C02_ADDR << 1), 
                               addr, 
                               I2C_MEMADD_SIZE_8BIT, 
                               &data, 
                               1, 
                               100);
    
    if (status != HAL_OK) {
        return 1;
    }
    
    /* 等待写入完成 */
    HAL_Delay(AT24C02_WRITE_DELAY);
    
    return 0;
}

/**
 * @brief 读取一个字节
 * @param addr: EEPROM内部地址 (0-255)
 * @param data: 读取的数据指针
 * @retval 0: 成功, 1: 失败
 */
uint8_t AT24C02_ReadByte(uint8_t addr, uint8_t *data)
{
    HAL_StatusTypeDef status;
    
    status = HAL_I2C_Mem_Read(&hi2c3, 
                              (AT24C02_ADDR << 1), 
                              addr, 
                              I2C_MEMADD_SIZE_8BIT, 
                              data, 
                              1, 
                              100);
    
    return (status != HAL_OK);
}

/**
 * @brief 保存音量到EEPROM
 * @param volume: 音量值 (0-100)
 * @retval 0: 成功, 1: 失败
 */
uint8_t AT24C02_SaveVolume(uint8_t volume)
{
    /* 写入有效标志 */
    if (AT24C02_WriteByte(AT24C02_FLAG_ADDR, AT24C02_VALID_FLAG) != 0) {
        return 1;
    }
    
    /* 写入音量值 */
    return AT24C02_WriteByte(AT24C02_VOLUME_ADDR, volume);
}

/**
 * @brief 从EEPROM读取音量
 * @param volume: 音量值指针
 * @retval 0: 成功读取有效值, 1: 无有效值或失败
 */
uint8_t AT24C02_LoadVolume(uint8_t *volume)
{
    uint8_t flag;
    uint8_t vol;
    
    /* 读取有效标志 */
    if (AT24C02_ReadByte(AT24C02_FLAG_ADDR, &flag) != 0) {
        return 1;
    }
    
    /* 检查标志是否有效 */
    if (flag != AT24C02_VALID_FLAG) {
        return 1;
    }
    
    /* 读取音量值 */
    if (AT24C02_ReadByte(AT24C02_VOLUME_ADDR, &vol) != 0) {
        return 1;
    }
    
    /* 检查音量范围 */
    if (vol > 100) {
        return 1;
    }
    
    *volume = vol;
    return 0;
}

uint8_t AT24C02_SaveVoltage(uint8_t voltage)
{
    if (AT24C02_WriteByte(AT24C02_VOLTAGE_FLAG_ADDR, AT24C02_VOLTAGE_VALID_FLAG) != 0) {
        return 1;
    }
    return AT24C02_WriteByte(AT24C02_VOLTAGE_ADDR, voltage);
}

uint8_t AT24C02_LoadVoltage(uint8_t *voltage)
{
    uint8_t flag;
    uint8_t volt;
    
    if (AT24C02_ReadByte(AT24C02_VOLTAGE_FLAG_ADDR, &flag) != 0) {
        return 1;
    }
    
    if (flag != AT24C02_VOLTAGE_VALID_FLAG) {
        return 1;
    }
    
    if (AT24C02_ReadByte(AT24C02_VOLTAGE_ADDR, &volt) != 0) {
        return 1;
    }
    
    if (volt > 2) {
        return 1;
    }
    
    *voltage = volt;
    return 0;
}

uint8_t AT24C02_SaveModulation(uint8_t modulation)
{
    if (AT24C02_WriteByte(AT24C02_MODULATION_FLAG_ADDR, AT24C02_MODULATION_VALID_FLAG) != 0) {
        return 1;
    }
    return AT24C02_WriteByte(AT24C02_MODULATION_ADDR, modulation);
}

uint8_t AT24C02_LoadModulation(uint8_t *modulation)
{
    uint8_t flag;
    uint8_t mod;
    
    if (AT24C02_ReadByte(AT24C02_MODULATION_FLAG_ADDR, &flag) != 0) {
        return 1;
    }
    
    if (flag != AT24C02_MODULATION_VALID_FLAG) {
        return 1;
    }
    
    if (AT24C02_ReadByte(AT24C02_MODULATION_ADDR, &mod) != 0) {
        return 1;
    }
    
    if (mod > 3) {
        return 1;
    }
    
    *modulation = mod;
    return 0;
}

uint8_t AT24C02_SaveFSW(uint8_t fsw)
{
    if (AT24C02_WriteByte(AT24C02_FSW_FLAG_ADDR, AT24C02_FSW_VALID_FLAG) != 0) {
        return 1;
    }
    return AT24C02_WriteByte(AT24C02_FSW_ADDR, fsw);
}

uint8_t AT24C02_LoadFSW(uint8_t *fsw)
{
    uint8_t flag;
    uint8_t freq;
    
    if (AT24C02_ReadByte(AT24C02_FSW_FLAG_ADDR, &flag) != 0) {
        return 1;
    }
    
    if (flag != AT24C02_FSW_VALID_FLAG) {
        return 1;
    }
    
    if (AT24C02_ReadByte(AT24C02_FSW_ADDR, &freq) != 0) {
        return 1;
    }
    
    if (freq > 5) {
        return 1;
    }
    
    *fsw = freq;
    return 0;
}

uint8_t AT24C02_SavePowerOff(uint8_t mode)
{
    if (AT24C02_WriteByte(AT24C02_POWEROFF_FLAG_ADDR, AT24C02_POWEROFF_VALID_FLAG) != 0) {
        return 1;
    }
    return AT24C02_WriteByte(AT24C02_POWEROFF_ADDR, mode);
}

uint8_t AT24C02_LoadPowerOff(uint8_t *mode)
{
    uint8_t flag;
    uint8_t m;
    
    if (AT24C02_ReadByte(AT24C02_POWEROFF_FLAG_ADDR, &flag) != 0) {
        return 1;
    }
    
    if (flag != AT24C02_POWEROFF_VALID_FLAG) {
        return 1;
    }
    
    if (AT24C02_ReadByte(AT24C02_POWEROFF_ADDR, &m) != 0) {
        return 1;
    }
    
    if (m > 3) {
        return 1;
    }
    
    *mode = m;
    return 0;
}

uint8_t AT24C02_SaveAMP(uint8_t gain)
{
    if (AT24C02_WriteByte(AT24C02_AMP_FLAG_ADDR, AT24C02_AMP_VALID_FLAG) != 0) {
        return 1;
    }
    return AT24C02_WriteByte(AT24C02_AMP_ADDR, gain);
}

uint8_t AT24C02_LoadAMP(uint8_t *gain)
{
    uint8_t flag;
    uint8_t g;
    
    if (AT24C02_ReadByte(AT24C02_AMP_FLAG_ADDR, &flag) != 0) {
        return 1;
    }
    
    if (flag != AT24C02_AMP_VALID_FLAG) {
        return 1;
    }
    
    if (AT24C02_ReadByte(AT24C02_AMP_ADDR, &g) != 0) {
        return 1;
    }
    
    if (g > 7) {
        return 1;
    }
    
    *gain = g;
    return 0;
}
