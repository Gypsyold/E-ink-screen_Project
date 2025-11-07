#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include "stm32f10x.h"                  // Device header
#include "screen_state.h"
#include "EPD_GUI.h"



extern uint8_t g_MenuSel;  // 主菜单选中项（0=日历，1=阅读）
extern uint16_t g_ReadingPage;   // 阅读当前页码（1~100）
void Display_Init(void);
void menu_ProcessTask(void *arg);


#endif

