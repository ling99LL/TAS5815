/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    menu.c
  * @brief   多级菜单系统实现
  ******************************************************************************
  * @attention
  * 
  * 功能说明：
  * - 默认显示音量，按键进入菜单
  * - 短按按键(PB9)确认/进入菜单
  * - 编码器转动选择菜单项
  * - 长按按键返回上一级菜单
  * 
  * STM32F411引脚定义：
  * - PB9: 编码器按键
* - PB12: 蓝牙配对/暂停
 * - PB13: VOL-/上一曲
 * - PB14: VOL+/下一曲
 * - PA9: PD 5V控制
 * - PB10: PD 20V控制
  * 
  ******************************************************************************
  */
/* USER CODE END Header */

#include "menu.h"
#include "oled.h"
#include "font.h"
#include "tas5815.h"
#include "at24c02.h"
#include <stdio.h>
#include <string.h>

/* 外部变量 */
extern TIM_HandleTypeDef htim2;

/* 菜单系统实例 */
static MenuSystem menuSys;

/* ========== 菜单项定义 ========== */

/* 子菜单项定义 */
/* 蓝牙设置功能函数 */
static void Bluetooth_Pair(void)
{
    HAL_GPIO_WritePin(BT_EN_GPIO_Port, BT_EN_Pin, GPIO_PIN_RESET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(BT_EN_GPIO_Port, BT_EN_Pin, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    HAL_Delay(1000);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
}

static void Bluetooth_PrevTrack(void)
{
    /* 上一曲：PB14输出0.1s高电平 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

static void Bluetooth_NextTrack(void)
{
    /* 下一曲：PB12输出0.1s高电平 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
}

/* 蓝牙设置子菜单 */
static MenuItem bluetoothMenu[] = {
    {"返回",       MENU_STATE_MAIN,      NULL, NULL, 0},
    {"蓝牙配对",   MENU_STATE_BLUETOOTH, Bluetooth_Pair, NULL, 0},
    {"上一曲",     MENU_STATE_BLUETOOTH, Bluetooth_PrevTrack, NULL, 0},
    {"下一曲",     MENU_STATE_BLUETOOTH, Bluetooth_NextTrack, NULL, 0},
};

/* 诱骗电压设置函数 */
static void Voltage_Set5V(void)
{
    SetVoltage(0);
    AT24C02_SaveVoltage(0);
}

static void Voltage_Set12V(void)
{
    SetVoltage(1);
    AT24C02_SaveVoltage(1);
}

static void Voltage_Set20V(void)
{
    SetVoltage(2);
    AT24C02_SaveVoltage(2);
}

/* 诱骗电压子菜单 */
static MenuItem voltageMenu[] = {
    {"返回",     MENU_STATE_MAIN,     NULL, NULL, 0},
    {"5V",       MENU_STATE_VOLTAGE,  Voltage_Set5V, NULL, 0},
    {"12V",      MENU_STATE_VOLTAGE,  Voltage_Set12V, NULL, 0},
    {"15V",      MENU_STATE_VOLTAGE,  Voltage_Set20V, NULL, 0},
};

/* AMP设置功能函数 */
static void AMP_SetGain0dB(void)   { tas5815_set_analog_gain(0); AT24C02_SaveAMP(0); }
static void AMP_SetGain2dB(void)   { tas5815_set_analog_gain(1); AT24C02_SaveAMP(1); }
static void AMP_SetGain4dB(void)   { tas5815_set_analog_gain(2); AT24C02_SaveAMP(2); }
static void AMP_SetGain6dB(void)   { tas5815_set_analog_gain(3); AT24C02_SaveAMP(3); }
static void AMP_SetGain8dB(void)   { tas5815_set_analog_gain(4); AT24C02_SaveAMP(4); }
static void AMP_SetGain10dB(void)  { tas5815_set_analog_gain(5); AT24C02_SaveAMP(5); }
static void AMP_SetGain12dB(void)  { tas5815_set_analog_gain(6); AT24C02_SaveAMP(6); }
static void AMP_SetGain14dB(void)  { tas5815_set_analog_gain(7); AT24C02_SaveAMP(7); }

/* AMP设置子菜单 */
static MenuItem ampMenu[] = {
    {"返回",        MENU_STATE_MAIN, NULL, NULL, 0},
    {"增益 0dB",    MENU_STATE_AMP, AMP_SetGain0dB, NULL, 0},
    {"增益 -2dB",   MENU_STATE_AMP, AMP_SetGain2dB, NULL, 0},
    {"增益 -4dB",   MENU_STATE_AMP, AMP_SetGain4dB, NULL, 0},
    {"增益 -6dB",   MENU_STATE_AMP, AMP_SetGain6dB, NULL, 0},
    {"增益 -8dB",   MENU_STATE_AMP, AMP_SetGain8dB, NULL, 0},
    {"增益 -10dB",  MENU_STATE_AMP, AMP_SetGain10dB, NULL, 0},
    {"增益 -12dB",  MENU_STATE_AMP, AMP_SetGain12dB, NULL, 0},
    {"增益 -14dB",  MENU_STATE_AMP, AMP_SetGain14dB, NULL, 0},
};

/* 调制模式设置功能函数 */
static void Modulation_SetBD(void)     { SetModulation(0); AT24C02_SaveModulation(0); }
static void Modulation_Set1SPW(void)   { SetModulation(1); AT24C02_SaveModulation(1); }
static void Modulation_SetHybrid(void) { SetModulation(3); AT24C02_SaveModulation(3); }

/* 调制模式子菜单 */
static MenuItem modulationMenu[] = {
    {"返回",        MENU_STATE_MAIN,       NULL, NULL, 0},
    {"BD 调制",     MENU_STATE_MODULATION, Modulation_SetBD, NULL, 0},
    {"1SPW 调制",   MENU_STATE_MODULATION, Modulation_Set1SPW, NULL, 0},
    {"Hybrid调制",  MENU_STATE_MODULATION, Modulation_SetHybrid, NULL, 0},
};

/* 开关频率设置功能函数 */
static void FSW_Set384k(void)   { SetFSW(1); AT24C02_SaveFSW(1); }
static void FSW_Set768k(void)   { SetFSW(0); AT24C02_SaveFSW(0); }


extern uint8_t g_powerOffMode;
extern void SetPowerOffMode(uint8_t mode);

static void PowerOff_Set10Min(void)      { SetPowerOffMode(0); AT24C02_SavePowerOff(0); }
static void PowerOff_Set30Min(void)      { SetPowerOffMode(1); AT24C02_SavePowerOff(1); }
static void PowerOff_SetFollowHost(void) { SetPowerOffMode(2); AT24C02_SavePowerOff(2); }
static void PowerOff_SetDisabled(void)   { SetPowerOffMode(3); AT24C02_SavePowerOff(3); }

/* 开关频率子菜单 */
static MenuItem fswMenu[] = {
    {"返回",        MENU_STATE_MAIN, NULL, NULL, 0},
    {"384kHz",      MENU_STATE_FSW,  FSW_Set384k, NULL, 0},
    {"768kHz",      MENU_STATE_FSW,  FSW_Set768k, NULL, 0},
};

static void PowerOff_Set10Min(void);
static void PowerOff_Set30Min(void);
static void PowerOff_SetFollowHost(void);
static void PowerOff_SetDisabled(void);

static MenuItem powerOffMenu[] = {
    {"返回",          MENU_STATE_MAIN,     NULL, NULL, 0},
    {"无播放10分钟",  MENU_STATE_POWEROFF, PowerOff_Set10Min, NULL, 0},
    {"无播放30分钟",  MENU_STATE_POWEROFF, PowerOff_Set30Min, NULL, 0},
    {"随主机关机",    MENU_STATE_POWEROFF, PowerOff_SetFollowHost, NULL, 0},
    {"不自动关机",    MENU_STATE_POWEROFF, PowerOff_SetDisabled, NULL, 0},
};

/* 一级主菜单 */
static MenuItem mainMenu[] = {
    {"返回",       MENU_STATE_VOLUME,     NULL, NULL,            0},
    {"蓝牙设置",   MENU_STATE_BLUETOOTH,  NULL, bluetoothMenu,   4},
    {"电压设置",   MENU_STATE_VOLTAGE,    NULL, voltageMenu,     4},
    {"模拟增益",   MENU_STATE_AMP,        NULL, ampMenu,         9},
    {"调制模式",   MENU_STATE_MODULATION, NULL, modulationMenu,  4},
    {"开关频率",   MENU_STATE_FSW,        NULL, fswMenu,         3},
    {"关机设置",   MENU_STATE_POWEROFF,   NULL, powerOffMenu,    5},
};

/* ========== 内部函数声明 ========== */
static void Menu_SwitchTo(MenuState state, MenuItem *menu, uint8_t size);
static void Menu_DrawItemList(MenuItem *items, uint8_t size, uint8_t selected, MenuState menuState);
static void Menu_ExecuteItem(MenuItem *item);

/* ========== 菜单系统函数实现 ========== */

/**
 * @brief 菜单系统初始化
 */
void Menu_Init(void)
{
    menuSys.currentState = MENU_STATE_VOLUME;
    menuSys.currentMenu = mainMenu;
    menuSys.currentMenuSize = MAIN_MENU_SIZE;
    menuSys.selectedIndex = 0;
    menuSys.encoderCount = 0;
    menuSys.inSubMenu = 0;
    
    /* 不再默认设置 12V，由 main.c 在启动时从 EEPROM 加载 */
}

/**
 * @brief 获取当前菜单状态
 */
MenuState Menu_GetState(void)
{
    return menuSys.currentState;
}

/**
 * @brief 进入菜单
 */
void Menu_Enter(void)
{
    menuSys.currentState = MENU_STATE_MAIN;
    menuSys.currentMenu = mainMenu;
    menuSys.currentMenuSize = MAIN_MENU_SIZE;
    menuSys.selectedIndex = 0;
    menuSys.inSubMenu = 0;
}

/**
 * @brief 退出菜单返回音量显示
 */
void Menu_Exit(void)
{
    menuSys.currentState = MENU_STATE_VOLUME;
    menuSys.selectedIndex = 0;
    menuSys.inSubMenu = 0;
    
    /* 同步硬件计数器为当前音量值，防止在其他界面旋转旋钮导致音量跳变 */
    SetVolume(GetVolume());
}

/**
 * @brief 切换到指定菜单
 */
static void Menu_SwitchTo(MenuState state, MenuItem *menu, uint8_t size)
{
    menuSys.currentState = state;
    menuSys.currentMenu = menu;
    menuSys.currentMenuSize = size;
    menuSys.selectedIndex = 0;
}

/**
 * @brief 绘制菜单项列表
 */
static void Menu_DrawItemList(MenuItem *items, uint8_t size, uint8_t selected, MenuState state)
{
    const uint8_t lineHeight = 16;
    const uint8_t startY = 16;
    const uint8_t maxVisibleItems = 3;
    
    uint8_t startIdx = 0;
    
    if (selected >= maxVisibleItems) {
        startIdx = selected - maxVisibleItems + 1;
    }
    
    uint8_t savedVoltage = 1;
    uint8_t savedModulation = 0;
    uint8_t savedFSW = 0;
    uint8_t savedAMP = 5;
    uint8_t savedPowerOff = 1;
    
    AT24C02_LoadVoltage(&savedVoltage);
    AT24C02_LoadModulation(&savedModulation);
    AT24C02_LoadFSW(&savedFSW);
    AT24C02_LoadAMP(&savedAMP);
    AT24C02_LoadPowerOff(&savedPowerOff);
    
    for (uint8_t i = 0; i < maxVisibleItems && (startIdx + i) < size; i++) {
        uint8_t idx = startIdx + i;
        uint8_t y = startY + i * lineHeight;
        
        if (idx == selected) {
            OLED_DrawFilledRectangle(0, y, 128, lineHeight, OLED_COLOR_NORMAL);
            OLED_PrintString(4, y + 2, (char *)items[idx].name, &font14x14, OLED_COLOR_REVERSED);
        } else {
            OLED_PrintString(4, y + 2, (char *)items[idx].name, &font14x14, OLED_COLOR_NORMAL);
        }
        
        uint8_t shouldDrawDot = 0;
        if (state == MENU_STATE_VOLTAGE && idx >= 1 && idx <= 3) {
            if ((idx == 1 && savedVoltage == 0) ||
                (idx == 2 && savedVoltage == 1) ||
                (idx == 3 && savedVoltage == 2)) {
                shouldDrawDot = 1;
            }
        }
        else if (state == MENU_STATE_AMP && idx >= 1 && idx <= 8) {
            if (savedAMP == (idx - 1)) {
                shouldDrawDot = 1;
            }
        }
        else if (state == MENU_STATE_MODULATION && idx >= 1 && idx <= 3) {
            if ((idx == 1 && savedModulation == 0) ||
                (idx == 2 && savedModulation == 1) ||
                (idx == 3 && savedModulation == 3)) {
                shouldDrawDot = 1;
            }
        }
        else if (state == MENU_STATE_FSW && idx >= 1 && idx <= 2) {
            if ((idx == 1 && savedFSW == 1) ||
                (idx == 2 && savedFSW == 0)) {
                shouldDrawDot = 1;
            }
        }
        else if (state == MENU_STATE_POWEROFF && idx >= 1 && idx <= 4) {
            if (savedPowerOff == (idx - 1)) {
                shouldDrawDot = 1;
            }
        }
        
        if (shouldDrawDot) {
            uint8_t dotX = 113;
            uint8_t dotY = y + 8;
            uint8_t dotR = 4;
            
            if (idx == selected) {
                OLED_DrawFilledCircle(dotX, dotY, dotR, OLED_COLOR_REVERSED);
            } else {
                OLED_DrawFilledCircle(dotX, dotY, dotR, OLED_COLOR_NORMAL);
            }
        }
    }
    
    if (size > maxVisibleItems) {
        if (startIdx > 0) {
            OLED_PrintASCIIString(120, 0, "^", &afont8x6, OLED_COLOR_NORMAL);
        }
        if (startIdx + maxVisibleItems < size) {
            OLED_PrintASCIIString(120, 56, "v", &afont8x6, OLED_COLOR_NORMAL);
        }
    }
}

/**
 * @brief 执行菜单项动作
 */
static void Menu_ExecuteItem(MenuItem *item)
{
    if (item->action != NULL) {
        item->action();
    }
    
    /* 如果有子菜单，进入子菜单 */
    if (item->subMenu != NULL && item->subMenuSize > 0) {
        Menu_SwitchTo(item->targetState, item->subMenu, item->subMenuSize);
        menuSys.inSubMenu = 1;
    }
    /* 如果是"返回"项 */
    else if (strcmp(item->name, "返回") == 0) {
        if (menuSys.inSubMenu) {
            /* 从子菜单返回主菜单 */
            Menu_SwitchTo(MENU_STATE_MAIN, mainMenu, MAIN_MENU_SIZE);
            menuSys.inSubMenu = 0;
        } else {
            /* 从主菜单返回音量显示 */
            Menu_Exit();
        }
    }
    /* 其他项目切换到目标状态 */
    else if (item->targetState != menuSys.currentState) {
        if (item->subMenu != NULL) {
            Menu_SwitchTo(item->targetState, item->subMenu, item->subMenuSize);
        } else {
            /* 执行后返回上一级 */
            if (menuSys.inSubMenu) {
                Menu_SwitchTo(MENU_STATE_MAIN, mainMenu, MAIN_MENU_SIZE);
                menuSys.inSubMenu = 0;
            }
        }
    }
}

/**
 * @brief 绘制当前菜单
 */
void Menu_Draw(void)
{
    OLED_NewFrame();
    
    switch (menuSys.currentState) {
        case MENU_STATE_VOLUME:
            break;
            
        case MENU_STATE_MAIN:
            OLED_PrintString(0, 0, "主菜单", &font14x14, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 14, 127, 14, OLED_COLOR_NORMAL);
            Menu_DrawItemList(menuSys.currentMenu, menuSys.currentMenuSize, menuSys.selectedIndex, MENU_STATE_MAIN);
            break;
            
        case MENU_STATE_BLUETOOTH:
            OLED_PrintString(0, 0, "蓝牙设置", &font14x14, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 14, 127, 14, OLED_COLOR_NORMAL);
            Menu_DrawItemList(menuSys.currentMenu, menuSys.currentMenuSize, menuSys.selectedIndex, MENU_STATE_BLUETOOTH);
            break;
            
        case MENU_STATE_VOLTAGE:
            OLED_PrintString(0, 0, "电压设置", &font14x14, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 14, 127, 14, OLED_COLOR_NORMAL);
            Menu_DrawItemList(menuSys.currentMenu, menuSys.currentMenuSize, menuSys.selectedIndex, MENU_STATE_VOLTAGE);
            break;
            
        case MENU_STATE_AMP:
            OLED_PrintString(0, 0, "模拟增益", &font14x14, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 14, 127, 14, OLED_COLOR_NORMAL);
            Menu_DrawItemList(menuSys.currentMenu, menuSys.currentMenuSize, menuSys.selectedIndex, MENU_STATE_AMP);
            break;
            
        case MENU_STATE_MODULATION:
            OLED_PrintString(0, 0, "调制模式", &font14x14, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 14, 127, 14, OLED_COLOR_NORMAL);
            Menu_DrawItemList(menuSys.currentMenu, menuSys.currentMenuSize, menuSys.selectedIndex, MENU_STATE_MODULATION);
            break;
            
        case MENU_STATE_FSW:
            OLED_PrintString(0, 0, "开关频率", &font14x14, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 14, 127, 14, OLED_COLOR_NORMAL);
            Menu_DrawItemList(menuSys.currentMenu, menuSys.currentMenuSize, menuSys.selectedIndex, MENU_STATE_FSW);
            break;
            
        case MENU_STATE_POWEROFF:
            OLED_PrintString(0, 0, "关机设置", &font14x14, OLED_COLOR_NORMAL);
            OLED_DrawLine(0, 14, 127, 14, OLED_COLOR_NORMAL);
            Menu_DrawItemList(menuSys.currentMenu, menuSys.currentMenuSize, menuSys.selectedIndex, MENU_STATE_POWEROFF);
            break;
    }
    
    OLED_ShowFrame();
}

/**
 * @brief 处理编码器输入
 * @param encoderDelta 编码器变化值
 */
void Menu_EncoderHandler(int32_t encoderDelta)
{
    if (menuSys.currentState == MENU_STATE_VOLUME) {
        return; /* 音量模式在main.c中处理 */
    }
    
    if (encoderDelta > 0) {
        /* 向下滚动 */
        if (menuSys.selectedIndex < menuSys.currentMenuSize - 1) {
            menuSys.selectedIndex++;
        }
    } else if (encoderDelta < 0) {
        /* 向上滚动 */
        if (menuSys.selectedIndex > 0) {
            menuSys.selectedIndex--;
        }
    }
}

/**
 * @brief 处理按键：短按确认/进入，长按退出
 */
void Menu_ButtonHandler(void)
{
    if (menuSys.currentState == MENU_STATE_VOLUME) {
        /* 音量状态按键进入菜单 */
        Menu_Enter();
    } else {
        /* 执行选中的菜单项 */
        if (menuSys.selectedIndex < menuSys.currentMenuSize) {
            Menu_ExecuteItem(&menuSys.currentMenu[menuSys.selectedIndex]);
        }
    }
}

static void Menu_DrawPowerOffProgress(uint32_t elapsed, uint32_t total)
{
    OLED_NewFrame();
    OLED_PrintString(39, 10, "关机", &font25x25, OLED_COLOR_NORMAL);
    
    uint8_t barY = 45;
    uint8_t barH = 12;
    uint8_t barW = 100;
    uint8_t barX = 14;
    
    OLED_DrawRectangle(barX, barY, barW, barH, OLED_COLOR_NORMAL);
    
    uint32_t progress = (elapsed * barW) / total;
    if (progress > 0 && progress <= barW) {
        OLED_DrawFilledRectangle(barX + 1, barY + 1, (uint8_t)progress - 2, barH - 2, OLED_COLOR_NORMAL);
    }
    
    OLED_ShowFrame();
}

static uint8_t g_powerOffPending = 0;
static uint8_t g_inPowerOffMode = 0;

uint8_t Menu_IsPowerOffPending(void)
{
    return g_powerOffPending;
}

void Menu_ClearPowerOffPending(void)
{
    g_powerOffPending = 0;
}

uint8_t Menu_IsInPowerOffMode(void)
{
    return g_inPowerOffMode;
}

/**
 * @brief 菜单处理函数（在主循环中调用的）
 */
void Menu_Process(void)
{
    static uint32_t lastEncoderCount = 0;
    static uint8_t lastButtonState = 1;
    static uint32_t buttonPressTime = 0;
    static uint8_t powerOffMode = 0;
    
    if (g_powerOffPending) {
        return;
    }
    
    uint32_t currentEncoder = __HAL_TIM_GET_COUNTER(&htim2);
    int32_t encoderDelta = (int32_t)currentEncoder - (int32_t)lastEncoderCount;
    
    if (encoderDelta > 32767) encoderDelta -= 65536;
    if (encoderDelta < -32767) encoderDelta += 65536;
    
    int32_t menuDelta = encoderDelta >> 1;
    
    if (menuDelta != 0) {
        Menu_EncoderHandler(menuDelta);
        lastEncoderCount = currentEncoder - (encoderDelta & 0x01);
    }
    
    uint8_t currentButtonState = HAL_GPIO_ReadPin(ENC_KEY_GPIO_Port, ENC_KEY_Pin);
    
    if (lastButtonState == 1 && currentButtonState == 0) {
        buttonPressTime = HAL_GetTick();
        powerOffMode = 0;
    }
    
    if (lastButtonState == 0 && currentButtonState == 0) {
        if (menuSys.currentState == MENU_STATE_VOLUME) {
            uint32_t pressDuration = HAL_GetTick() - buttonPressTime;
            if (pressDuration >= 1000) {
                powerOffMode = 1;
                g_inPowerOffMode = 1;
                uint32_t powerOffProgress = pressDuration - 1000;
                if (powerOffProgress >= 2000) {
                    g_powerOffPending = 1;
                    g_inPowerOffMode = 0;
                    return;
                }
                Menu_DrawPowerOffProgress(powerOffProgress, 2000);
            }
        }
    }
    
    if (lastButtonState == 0 && currentButtonState == 1) {
        uint32_t pressDuration = HAL_GetTick() - buttonPressTime;
        
        if (powerOffMode) {
            powerOffMode = 0;
            g_inPowerOffMode = 0;
        }
        else if (pressDuration > 1000) {
            if (menuSys.currentState != MENU_STATE_VOLUME) {
                if (menuSys.inSubMenu) {
                    Menu_SwitchTo(MENU_STATE_MAIN, mainMenu, MAIN_MENU_SIZE);
                    menuSys.inSubMenu = 0;
                } else {
                    Menu_Exit();
                }
            }
        } else if (pressDuration > 20) {
            Menu_ButtonHandler();
        }
    }
    
    lastButtonState = currentButtonState;
    
    if (menuSys.currentState != MENU_STATE_VOLUME && !powerOffMode) {
        Menu_Draw();
    }
}
