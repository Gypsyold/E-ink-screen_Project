//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//中景园电子
//店铺地址：http://shop73023976.taobao.com
//
//  文 件 名   : main.c
//  版 本 号   : v2.0
//  作    者   : ZhaoJian
//  生成日期   : 2023-11-30
//  最近修改   : 
//  功能描述   :演示例程(STM32F407系列)
//              说明: 
//              ----------------------------------------------------------------
//              GND   电源地
//              VCC   3.3v电源
//              SCL   PG12（SCLK）
//              SDA   PD5（MOSI）
//              RES   PD4
//              DC    PD15
//              CS    PD1
//              BLK   PE8
//              ----------------------------------------------------------------
// 修改历史   :
// 日    期   : 
// 作    者   : ZhaoJian
// 修改内容   : 创建文件
//版权所有，盗版必究。
//Copyright(C) 中景园电子2023-11-30
//All rights reserved
//******************************************************************************/

#include "delay.h"
#include "usart.h"
#include "EPD_GUI.h"
#include "Pic.h"


u8 ImageBW[5624];

int main()
{
	float num=12.05;
	u8 dat=0;
	delay_init(168);
	uart_init(115200);
	EPD_GPIOInit();
	/************************全刷************************/
	EPD_Init();
	EPD_Display(gImage_1);
	EPD_Update();
	EPD_DeepSleep();
	delay_ms(1000);
  /*********************快刷模式**********************/
//	EPD_FastInit();
//	EPD_Display(gImage_2);
//	EPD_FastUpdate();
//	EPD_DeepSleep();
//	delay_ms(1000);
//	/************************全刷************************/
//	EPD_Init();
//	Paint_NewImage(ImageBW,EPD_W,EPD_H,0,WHITE);		//创建画布
//	Paint_Clear(WHITE);	
//	EPD_Display_Clear();		
//	EPD_ShowPicture(40,0,216,112,gImage_3,BLACK);
//	EPD_ShowString(36,130,"2.66inch",16,BLACK);
//	EPD_ShowChinese(100,130,"电子墨水屏断电可显示",16,BLACK);
//	EPD_Display(ImageBW);
//	EPD_Update();
//	EPD_DeepSleep();
//	Paint_Clear(WHITE);  		//清除画布缓存
//	delay_ms(1000);
//	/************************先全刷清屏然后工作在局刷模式************************/
//	EPD_Init();
//	EPD_Display_Clear();
//	EPD_Update();						//更新画面显示
//	EPD_Clear_R26H();
//	while(1)
//	{
//		/*********************局刷模式**********************/
//		EPD_ShowString(33,0,"Welcome to 2.66-inch E-paper",16,BLACK);		
//		EPD_ShowString(49,20,"with 296 x 152 resolution",16,BLACK);	
//		EPD_ShowString(68,40,"Demo-Test-2023/10/16",16,BLACK);
//		EPD_DrawLine(0,60,295,60,BLACK);
//		EPD_DrawRectangle(4,78,44,118,BLACK,0);
//		EPD_DrawRectangle(48,78,88,119,BLACK,1);
//		EPD_ShowWatch(88,74,num,4,2,48,BLACK);
//		EPD_DrawCircle(245,100,25,BLACK,0);
//		EPD_DrawCircle(265,100,25,BLACK,1);
//		EPD_ShowChinese(44,136,"郑州中景园电子科技有限公司",16,BLACK);
//		EPD_DrawRectangle(0,0,295,151,BLACK,0);
//		num+=0.01;
//		EPD_Display(ImageBW);
//		EPD_PartUpdate();
//		delay_ms(500);
//		dat++;
//		if(dat==5)
//		{
//			EPD_Display_Clear();
//			EPD_Update();
//			EPD_DeepSleep();
//		}
//	}
}


