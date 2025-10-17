#include "Menu.h"
#include "EPD_GUI.h"
#include "Key.h"
#include "LED.h"


// 按矩形区域取反：从起点(x,y)开始，宽w，高h
void EPD_InvertRect(u8 *image, u16 x, u16 y, u16 w, u16 h)
{
	/* 以显示坐标解释参数，并自动适配旋转，将其映射到显存坐标后取反 */
	u16 Xd, Yd;          /* 显示坐标 */
	u16 Xm, Ym;          /* 显存坐标（与Paint_SetPixel一致的映射） */
	u16 xEnd, yEnd;      /* 显示坐标的裁剪后终点 */
	u16 dispMaxX, dispMaxY;  /* 显示坐标系下的最大范围 */
	unsigned int rowBytes;   
	unsigned int addr;   
	u8 mask; 

	/* 基于旋转计算显示坐标可用范围（与Paint_SetPixel相同的显示坐标系） */
	if(Paint.rotate == 0 || Paint.rotate == 180)
	{
		dispMaxX = Paint.heightMemory; /* 横向长度对应显存Y维度 */
		dispMaxY = Paint.widthMemory;  /* 纵向高度对应显存X维度 */
	}
	else
	{
		dispMaxX = Paint.widthMemory;
		dispMaxY = Paint.heightMemory;
	}
	if(w == 0 || h == 0) return;
	xEnd = x + w; if(xEnd > dispMaxX) xEnd = dispMaxX;
	yEnd = y + h; if(yEnd > dispMaxY) yEnd = dispMaxY;
	if(x >= xEnd || y >= yEnd) return;

	/* 显存行宽与地址使用Paint参数，避免与物理常量不一致 */
	if(Paint.widthByte == 0) return;
	rowBytes = Paint.widthByte;

	for(Yd = y; Yd < yEnd; Yd++)
	{
		for(Xd = x; Xd < xEnd; Xd++)
		{
			/* 将显示坐标映射到显存坐标，逻辑与Paint_SetPixel的rotate对应 */
			switch(Paint.rotate)
			{
				case 0:
					Xm = Yd;
					Ym = Xd;
					break;
				case 90:
					Xm = Xd;
					Ym = (Paint.heightMemory > 0 ? (Paint.heightMemory - Yd - 1) : 0);
					break;
				case 180:
					Xm = (Paint.widthMemory > 0 ? (Paint.widthMemory - Yd - 1) : 0);
					Ym = (Paint.heightMemory > 0 ? (Paint.heightMemory - Xd - 1) : 0);
					break;
				case 270:
					Xm = (Paint.widthMemory > 0 ? (Paint.widthMemory - Xd - 1) : 0);
					Ym = Yd;
					break;
				default:
					return;
			}

			/* 防越界（显存维度） */
			if(Xm >= Paint.widthMemory || Ym >= Paint.heightMemory) continue;

			addr = (Xm / 8) + (unsigned int)Ym * rowBytes;
			mask = (u8)(0x80 >> (Xm % 8));
			image[addr] ^= mask;
		}
	}
}

uint8_t KeyNum = 0;
uint8_t flag_position = 1;
uint8_t FlashFlag = 0;
int menu1(void)
{
	Paint_Clear(WHITE);
	EPD_ShowChinese(0,0,(u8 *)"风扇控制             ",24,BLACK);
	EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
	EPD_ShowChinese(0,48,(u8 *)"遥杆数据            ",24,BLACK);
	EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);

	EPD_Display(ImageBW);
	EPD_PartUpdate();

	while(1)
	{
		KeyNum = Key_GetNum();
	
		
		if(KeyNum == 1)	// 下一项
		{
			flag_position ++;
			if(flag_position == 9){flag_position = 1;}
		
		}
		if(KeyNum == 2)	// 上一项
		{
			flag_position --;
			if(flag_position == 0){flag_position = 8;}
		
		}
		if(KeyNum == 3) // 确认
		{
			EPD_Display_Clear();
			EPD_Update();
			return flag_position;
		}
		
		if(flag_position % 2 == 1)
		{

			LED1_SetMode(1);
		
		
		}else
		{
			LED1_SetMode(0);

		}
		
		
		switch(flag_position)
		{
			case 1:
			{
					Paint_Clear(WHITE);
					EPD_ShowChinese(0,0,(u8 *)"风扇控制             ",24,BLACK);
					EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"遥杆数据            ",24,BLACK);
					EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);
					EPD_InvertRect(ImageBW,0,0,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					
					break;
				
				
			}
			case 2:
			{
					Paint_Clear(WHITE);
					EPD_ShowChinese(0,0,(u8 *)"风扇控制             ",24,BLACK);
					EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"遥杆数据            ",24,BLACK);
					EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);
					EPD_InvertRect(ImageBW,0,24,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;				
			}			
			case 3:
			{
					Paint_Clear(WHITE);
					EPD_ShowChinese(0,0,(u8 *)"风扇控制             ",24,BLACK);
					EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"遥杆数据            ",24,BLACK);
					EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);
					EPD_InvertRect(ImageBW,0,48,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();	
					break;				
			}
			case 4:
			{
					Paint_Clear(WHITE);
					EPD_ShowChinese(0,0,(u8 *)"风扇控制             ",24,BLACK);
					EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"遥杆数据            ",24,BLACK);
					EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);
					EPD_InvertRect(ImageBW,0,72,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;				
			}
			case 5:
			{
					Paint_Clear(WHITE);
					EPD_ShowString (0,0,(u8 *)"PID                  ",24,BLACK);
					EPD_ShowChinese(0,24,(u8 *)"时钟                ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"音乐                ",24,BLACK);
					EPD_ShowChinese(0,72,(u8 *)"设置                ",24,BLACK);
					EPD_InvertRect(ImageBW,0,0,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;					
			}
			case 6:
			{
					Paint_Clear(WHITE);
					EPD_ShowString (0,0,(u8 *)"PID                  ",24,BLACK);
					EPD_ShowChinese(0,24,(u8 *)"时钟                ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"音乐                ",24,BLACK);
					EPD_ShowChinese(0,72,(u8 *)"设置                ",24,BLACK);
					EPD_InvertRect(ImageBW,0,24,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();	
					break;				
			}
			case 7:
			{
					Paint_Clear(WHITE);
					EPD_ShowString (0,0,(u8 *)"PID                  ",24,BLACK);
					EPD_ShowChinese(0,24,(u8 *)"时钟                ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"音乐                ",24,BLACK);
					EPD_ShowChinese(0,72,(u8 *)"设置                ",24,BLACK);
					EPD_InvertRect(ImageBW,0,48,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;					
			}
			case 8:
			{
					Paint_Clear(WHITE);
					EPD_ShowString (0,0,(u8 *)"PID                  ",24,BLACK);
					EPD_ShowChinese(0,24,(u8 *)"时钟                ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"音乐                ",24,BLACK);
					EPD_ShowChinese(0,72,(u8 *)"设置                ",24,BLACK);
					EPD_InvertRect(ImageBW,0,72,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;				
			}

		
		}
	}


}

