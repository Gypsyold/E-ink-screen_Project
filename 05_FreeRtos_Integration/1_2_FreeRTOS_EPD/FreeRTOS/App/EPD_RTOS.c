//#include "stm32f10x.h"                  // Device header
//#include "EPD_RTOS.h"
//#include "menu_Pic.h"
//#include "EPD_GUI.h"
//#include "delay.h"
//#include "task.h"
//#include "semphr.h"





//uint8_t ImageBW[5624];
//void menu_ProcessTask(void *arg)
//{
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// 绑定画布

//	Paint_Clear(WHITE);

//	// 上电先全屏清白并全刷一次，去残影
//	EPD_Display_Clear();
//	EPD_Update();
//	EPD_Clear_R26H();										// 进入局刷对比模式
//	Delay_ms(1000);
//	
//	while(1)
//	{
//		Paint_Clear(WHITE);
//		EPD_ShowPicture(0,0,EPD_H,EPD_W,gImage_two_menu_choose,WHITE);	
//		EPD_ShowChinese(23,121,(u8 *)"日历模式",24,BLACK);
//		EPD_ShowChinese(171,121,(u8 *)"阅读模式",24,BLACK);		
//		
//		EPD_InvertRect(ImageBW,0,0,EPD_H/2,EPD_W);
//		
//		EPD_Display(ImageBW);
//		EPD_PartUpdate();	
//	
//	}

//}


