/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tas5815.c
  * @brief   TAS5815 driver functions
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 Your Company/Name.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "tas5815.h"
#include <stdint.h>

/* TAS5815 I2C Address - 7-bit address is 0x54 (ADR pin with 4.7k to DVDD) */
#define TAS5815_ADDR        (0x54 << 1)  // 8-bit write address: 0xA8

/* Register addresses */
#define TAS5815_REG_RESET           0x01  // Reset Control
#define TAS5815_REG_DEVICE_CTRL1    0x02  // Device Control 1
#define TAS5815_REG_DEVICE_CTRL2    0x03  // Device Control 2
#define TAS5815_REG_SAP_CTRL1       0x33  // I2S Control 1
#define TAS5815_REG_ANA_CTRL        0x53  // Analog Control
#define TAS5815_REG_AGAIN           0x54  // Analog Gain
#define TAS5815_REG_DIG_VOL         0x4C  // Digital Volume

/************************ (C) COPYRIGHT Your Company *****END OF FILE****/

extern I2C_HandleTypeDef hi2c1; // I2C1 handle

static uint8_t current_device_ctrl1 = 0x00;  /* Track current DEVICE_CTRL1 register value */

/**
  * @brief  Millisecond delay function
  * @param  ms: delay in milliseconds
  * @retval None
  */
void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

/**
  * @brief  Write a single register to TAS5815
  * @param  reg: Register address
  * @param  value: Value to write
  * @retval HAL status
  */
static HAL_StatusTypeDef TAS5815_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t data[2];
    data[0] = reg;
    data[1] = value;
    return HAL_I2C_Master_Transmit(&hi2c1, TAS5815_ADDR, data, 2, 100);
}

/**
  * @brief  Initialize TAS5815 amplifier
  * @note   Initialization sequence based on TAS5815 datasheet:
  *         1. Wait for power stable (5ms)
  *         2. Set to Hi-Z mode with DSP reset
  *         3. Configure I2S format
  *         4. Configure PWM frequency and modulation
  *         5. Configure analog settings
  *         6. Set volume
  *         7. Enter play mode
  * @retval None
  */
void tas5815_Initialize(void)
{
    HAL_StatusTypeDef status;
    
    /* Step 1: Wait for power stable after PDN goes high */
    delay_ms(5);
    
    /* Step 2: Set device to Hi-Z mode with DSP reset (DSP_RST=1, STATE=10 Hi-Z) */
    /* Register 0x03: bit4=DSP_RST(1), bit3=MUTE(0), bits[1:0]=STATE(10=Hi-Z) */
    /* Value = 0x10 | 0x02 = 0x12, but we want DSP reset first, so 0x10 */
    status = TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x10);
    if (status != HAL_OK) {
        Error_Handler();
    }
    delay_ms(2);
    
    /* Step 3: Configure I2S format - Register 0x33 (SAP_CTRL1) */
    /* Bits[5:4] = 00: I2S format */
    /* Bits[1:0] = 10: 24-bit word length */
    /* Value = 0x02 for I2S, 24-bit */
    status = TAS5815_WriteReg(TAS5815_REG_SAP_CTRL1, 0x02);
    if (status != HAL_OK) {
        Error_Handler();
    }
    delay_ms(1);
    
    /* Step 4: Configure Device Control 1 - Register 0x02 */
    /* Bits[6:4] = 000: FSW = 768kHz */
    /* Bit[2] = 0: BTL mode */
    /* Bits[1:0] = 00: BD modulation */
    /* Value = 0x00 for 768kHz, BTL, BD modulation */
    status = TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL1, 0x00);
    if (status != HAL_OK) {
        Error_Handler();
    }
    delay_ms(1);
    
    /* Step 5: Configure D-class loop bandwidth - Register 0x53 (ANA_CTRL) */
    /* Bits[6:5] = 11: 175kHz bandwidth (recommended for 768kHz FSW) */
    /* Value = 0x60 */
    status = TAS5815_WriteReg(TAS5815_REG_ANA_CTRL, 0x60);
    if (status != HAL_OK) {
        Error_Handler();
    }
    delay_ms(1);
    
    /* Step 6: Set analog gain - Register 0x54 (AGAIN) */
    /* Bits[4:0] = 00000: 0dB analog gain */
    /* Value = 0x00 for 0dB */
    status = TAS5815_WriteReg(TAS5815_REG_AGAIN, 0x00);
    if (status != HAL_OK) {
        Error_Handler();
    }
    delay_ms(1);
    
    /* Step 7: Set digital volume - Register 0x4C (DIG_VOL_CTL) */
    /* 0x30 = 0dB volume (default) */
    /* Range: 0x00=+24dB to 0xFF=mute, 0x30=0dB */
    status = TAS5815_WriteReg(TAS5815_REG_DIG_VOL, 0x30);
    if (status != HAL_OK) {
        Error_Handler();
    }
    delay_ms(1);
    
    /* Step 8: Enter Play mode with DSP enabled */
    /* Register 0x03: bit4=DSP_RST(0), bit3=MUTE(0), bits[1:0]=STATE(11=Play) */
    /* Value = 0x03 for Play mode */
    status = TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x03);
    if (status != HAL_OK) {
        Error_Handler();
    }
    delay_ms(5);
}

/**
  * @brief  Set TAS5815 digital volume
  * @param  count: Volume level (0-100, 0=mute, 100=max)
  * @note   Digital volume range: +24dB to -103dB in 0.5dB steps
  *         Register 0x00 = +24dB
  *         Register 0x30 = 0dB
  *         Register 0xFF = Mute
  * @retval None
  */
void set_tas5815_volume(uint32_t count)
{
    uint8_t reg_val;
    
    if (count == 0) {
        /* Mute */
        reg_val = 0xFF;
    } else if (count >= 100) {
        /* Maximum volume: +24dB */
        reg_val = 0x00;
    } else {
        /* Map 1-99 to volume range
         * count 1 = -50dB (reg 0x64)
         * count 50 = 0dB (reg 0x30)
         * count 99 = +23.5dB (reg 0x01)
         * Linear mapping: reg = 0x64 - (count - 1) * 100 / 98
         */
        int32_t vol_steps = (int32_t)((100 - count) * 100 / 99);
        reg_val = (uint8_t)(0x30 + vol_steps - 50);
        
        /* Clamp to valid range */
        if (reg_val > 0xFE && count > 0) {
            reg_val = 0x00;  /* Max volume */
        }
    }
    
    TAS5815_WriteReg(TAS5815_REG_DIG_VOL, reg_val);
}

/**
  * @brief  Mute TAS5815 output
  * @retval None
  */
void tas5815_mute(void)
{
    TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x0B);  /* MUTE=1, PLAY mode */
}

/**
  * @brief  Unmute TAS5815 output
  * @retval None
  */
void tas5815_unmute(void)
{
    TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x03);  /* MUTE=0, PLAY mode */
}

/**
  * @brief  Set TAS5815 analog gain
  * @param  gain_index: Gain index (0-7)
  *         0 = 0dB   (AGAIN = 0x00)
  *         1 = -2dB  (AGAIN = 0x04)
  *         2 = -4dB  (AGAIN = 0x08)
  *         3 = -6dB  (AGAIN = 0x0C)
  *         4 = -8dB  (AGAIN = 0x10)
  *         5 = -10dB (AGAIN = 0x14)
  *         6 = -12dB (AGAIN = 0x18)
  *         7 = -14dB (AGAIN = 0x1C)
  * @note   Analog gain range: 0dB to -15.5dB in 0.5dB steps
  *         Register AGAIN[4:0] = gain_dB * 2 (each step = 0.5dB)
  * @retval None
  */
void tas5815_set_analog_gain(uint8_t gain_index)
{
    /* Analog gain table: index -> AGAIN register value */
    /* Each 2dB step = 4 * 0.5dB steps */
    static const uint8_t gain_table[8] = {
        0x00,  /* 0dB   */
        0x04,  /* -2dB  */
        0x08,  /* -4dB  */
        0x0C,  /* -6dB  */
        0x10,  /* -8dB  */
        0x14,  /* -10dB */
        0x18,  /* -12dB */
        0x1C   /* -14dB */
    };
    
    if (gain_index >= 8) {
        gain_index = 5;  /* Default to -10dB */
    }
    
    TAS5815_WriteReg(TAS5815_REG_AGAIN, gain_table[gain_index]);
}

/**
  * @brief  Set TAS5815 modulation mode
  * @param  modulation: Modulation mode
  *         0 = BD modulation
  *         1 = 1SPW modulation
  *         3 = Hybrid modulation
  * @note   Register 0x02 (DEVICE_CTRL1) bits[1:0] control modulation
  *         Hybrid mode requires special sequence: BD/1SPW -> Hi-Z -> Hybrid -> Play
  * @retval None
  */
void tas5815_set_modulation(uint8_t modulation)
{
    uint8_t val = current_device_ctrl1 & ~0x03;
    val |= (modulation & 0x03);

    if (modulation == 0x03) {
        TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL1, current_device_ctrl1 & ~0x03);
        TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x12);
        delay_ms(2);
        TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL1, val);
        delay_ms(15);
        TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x03);
        delay_ms(5);
    } else {
        TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL1, val);
    }

    current_device_ctrl1 = val;
}

/**
  * @brief  Set TAS5815 PWM switch frequency
  * @param  fsw_sel: Frequency selection
  *         0 = 768kHz
  *         1 = 384kHz
  * @note   Register 0x02 (DEVICE_CTRL1) bits[6:4] control FSW_SEL
  *         Also updates D-class loop bandwidth (ANA_CTRL) for optimal performance
  *         384kHz -> 120kHz bandwidth, 768kHz/1.024MHz -> 175kHz bandwidth
  * @retval None
  */
void tas5815_set_fsw(uint8_t fsw_sel)
{
    uint8_t val = current_device_ctrl1 & ~0x70;
    val |= ((fsw_sel & 0x07) << 4);

    TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL1, val);

    if (fsw_sel == 1) {
        TAS5815_WriteReg(TAS5815_REG_ANA_CTRL, 0x00);
    } else {
        TAS5815_WriteReg(TAS5815_REG_ANA_CTRL, 0x60);
    }

    current_device_ctrl1 = val;
}

void tas5815_sleep(void)
{
    TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x00);
}

void tas5815_wakeup(void)
{
    TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x02);
    delay_ms(5);
    TAS5815_WriteReg(TAS5815_REG_DEVICE_CTRL2, 0x03);
    delay_ms(5);
}
