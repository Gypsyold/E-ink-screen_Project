#include "Display.h"
#include "screen_state.h"
#include "EPD_GUI.h"
#include "menu_Pic.h"

// 显示任务局部状态（各画面的临时变量）
uint8_t g_MenuSel = 0;        // 主菜单选中项（0=日历，1=阅读）- 改为全局供按键任务查询
static uint16_t g_ReadingPage = 1;   // 阅读当前页码（1~100）
uint8_t ImageBW[5624];

// 水墨屏初始化
void Display_Init(void) 
{
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
    Paint_Clear(WHITE);
    EPD_Display_Clear();
	EPD_ShowPicture(0,0,EPD_H,EPD_W,gImage_two_menu_choose,WHITE);	
	EPD_ShowChinese(23,121,(u8 *)"日历模式",24,BLACK);
	EPD_ShowChinese(171,121,(u8 *)"阅读模式",24,BLACK);		
	
	
    EPD_Update();
    EPD_Clear_R26H(); // 局刷模式
    vTaskDelay(pdMS_TO_TICKS(1000));
}


extern QueueHandle_t xDispCmdQueue;
// 刷新显示（根据当前画面绘制内容）
static void Display_Refresh(CurrentScreen screen) 
{
    Paint_Clear(WHITE); // 清屏

    switch (screen) 
	{
        case SCREEN_MAIN_MENU:
            // 主菜单：显示选项并高亮选中项
            EPD_ShowPicture(0,0,EPD_H,EPD_W,gImage_two_menu_choose,WHITE);	
			EPD_ShowChinese(23,121,(u8 *)"日历模式",24,BLACK);
			EPD_ShowChinese(171,121,(u8 *)"阅读模式",24,BLACK);	
            
			if(g_MenuSel == 0) EPD_InvertRect(ImageBW,0,0,EPD_H/2,EPD_W);
			else EPD_InvertRect(ImageBW,EPD_H/2,0,EPD_H/2,EPD_W);
			
            break;

        case SCREEN_READING_SEL:
            // 阅读-页码选择
            EPD_ShowChinese(50, 30, (u8 *)"阅读", 24, BLACK);
           
            break;

        case SCREEN_READING_VIEW:
            // 阅读-内容显示
            EPD_ShowChinese(50, 30, (u8 *)"阅读内容", 24, BLACK);

            // 示例内容（实际项目中可从数组/Flash读取）

            break;

        case SCREEN_CALENDAR:
            // 日历模式
            EPD_ShowChinese(50, 30, (u8 *)"日历", 24, BLACK);

            break;
    }

    // 刷新屏幕（局刷）
    EPD_Display(ImageBW);
    EPD_PartUpdate();
}

// 显示任务：接收命令→更新状态→刷新显示
void menu_ProcessTask(void *arg) 
{
    Display_Init(); 
    CurrentScreen curScreen = SCREEN_MAIN_MENU;
    DispCmd cmd;

    while (1) 
	{
        // 接收显示命令（阻塞等待）
        if (xQueueReceive(xDispCmdQueue, &cmd, portMAX_DELAY) != pdPASS) continue;

        // 根据命令更新状态
        switch (cmd.cmd) 
			{
            // 主菜单命令
            case DISP_CMD_MENU_SEL_CHANGE:
                g_MenuSel = cmd.param;
                curScreen = SCREEN_MAIN_MENU;
                break;
            case DISP_CMD_ENTER_CALENDAR:
                curScreen = SCREEN_CALENDAR;
                break;
            case DISP_CMD_ENTER_READING_SEL:
                curScreen = SCREEN_READING_SEL;
                break;

            // 阅读模式命令
            case DISP_CMD_READING_PAGE_CHANGE:
                g_ReadingPage += cmd.param;
                if (g_ReadingPage < 1) g_ReadingPage = 1;
                if (g_ReadingPage > 100) g_ReadingPage = 100;
                curScreen = SCREEN_READING_SEL;
                break;
            case DISP_CMD_ENTER_READING_VIEW:
                curScreen = SCREEN_READING_VIEW;
                break;
            case DISP_CMD_BACK_TO_READING_SEL:
                curScreen = SCREEN_READING_SEL;
                break;

            // 通用命令
            case DISP_CMD_BACK_TO_MAIN:
                g_MenuSel = 0; // 重置主菜单选中项
                curScreen = SCREEN_MAIN_MENU;
                break;

            default:
                break;
        }

        // 同步全局画面状态（供按键任务查询）
        ScreenState_Set(curScreen);
        // 刷新显示
        Display_Refresh(curScreen);
    }
}
