#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include "stm32f10x.h"                  // Device header

extern uint8_t g_MenuSel;  // 主菜单选中项（0=日历，1=阅读）
void Display_Init(void);
void menu_ProcessTask(void *arg);


#endif

