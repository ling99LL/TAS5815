/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    menu.h
  * @brief   多级菜单系统头文件
  ******************************************************************************
  * @attention
  * 多级菜单系统，支持编码器选择和按键确认
  * 开机默认为音量调节界面，长按按键进入菜单
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __MENU_H
#define __MENU_H

#include "main.h"
#include <stdint.h>

#define MAIN_MENU_SIZE  7  /* 主菜单项数量 */

/* 菜单状态枚举 */
typedef enum {
    MENU_STATE_VOLUME = 0,    /* 音量调节界面（默认） */
    MENU_STATE_MAIN,          /* 一级菜单 */
    MENU_STATE_BLUETOOTH,     /* 蓝牙设置子菜单 */
    MENU_STATE_VOLTAGE,       /* 诱骗电压子菜单 */
    MENU_STATE_AMP,           /* AMP设置子菜单 */
    MENU_STATE_MODULATION,    /* 调制模式子菜单 */
    MENU_STATE_FSW,           /* 开关频率子菜单 */
    MENU_STATE_POWEROFF       /* 关机设置子菜单 */
} MenuState;

/* 菜单项结构体 */
typedef struct MenuItem {
    const char *name;                    /* 菜单项名称 */
    MenuState targetState;               /* 目标状态 */
    void (*action)(void);                /* 执行函数（可选） */
    struct MenuItem *subMenu;            /* 子菜单指针 */
    uint8_t subMenuSize;                 /* 子菜单大小 */
} MenuItem;

/* 菜单系统结构体 */
typedef struct {
    MenuState currentState;              /* 当前菜单状态 */
    MenuItem *currentMenu;               /* 当前菜单项数组 */
    uint8_t currentMenuSize;             /* 当前菜单项数量 */
    uint8_t selectedIndex;               /* 当前选中索引 */
    int32_t encoderCount;                /* 编码器计数缓存 */
    uint8_t inSubMenu;                   /* 是否在子菜单中 */
} MenuSystem;

/* 菜单系统初始化 */
void Menu_Init(void);

/* 菜单主处理函数 */
void Menu_Process(void);

/* 获取当前菜单状态 */
MenuState Menu_GetState(void);

/* 进入菜单 */
void Menu_Enter(void);

/* 退出菜单返回音量界面 */
void Menu_Exit(void);

/* 绘制当前菜单 */
void Menu_Draw(void);

/* 编码器处理 */
void Menu_EncoderHandler(int32_t encoderDelta);

/* 按键处理 */
void Menu_ButtonHandler(void);

/* 关机状态查询 */
uint8_t Menu_IsPowerOffPending(void);
void Menu_ClearPowerOffPending(void);
uint8_t Menu_IsInPowerOffMode(void);

#endif /* __MENU_H */
