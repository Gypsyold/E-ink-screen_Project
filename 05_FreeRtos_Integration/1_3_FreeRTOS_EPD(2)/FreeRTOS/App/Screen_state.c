#include "Screen_state.h"




// 全局变量：当前画面状态（需互斥锁保护）
static CurrentScreen g_CurScreen = SCREEN_MAIN_MENU;
static SemaphoreHandle_t g_ScreenMutex; // 保护画面状态的互斥锁

// 初始化画面状态和互斥锁
void ScreenState_Init(void) 
	


{
    g_ScreenMutex = xSemaphoreCreateMutex();
    configASSERT(g_ScreenMutex != NULL); // 确保互斥锁创建成功
    g_CurScreen = SCREEN_MAIN_MENU;
}

// 获取当前画面状态（供按键任务查询）
CurrentScreen ScreenState_Get(void) 
{
    CurrentScreen temp = SCREEN_MAIN_MENU;
    if (xSemaphoreTake(g_ScreenMutex, pdMS_TO_TICKS(10)) == pdTRUE) 
	{
        temp = g_CurScreen;
        xSemaphoreGive(g_ScreenMutex);
    }
    return temp;
}

// 更新当前画面状态（供显示任务调用）
void ScreenState_Set(CurrentScreen newScreen) 
{
    if (xSemaphoreTake(g_ScreenMutex, pdMS_TO_TICKS(10)) == pdTRUE) 
	{
        g_CurScreen = newScreen;
        xSemaphoreGive(g_ScreenMutex);
    }
}

