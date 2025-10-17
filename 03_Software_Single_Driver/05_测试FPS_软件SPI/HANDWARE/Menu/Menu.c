#include "Menu.h"
#include "EPD_GUI.h"
#include "Key.h"
#include "LED.h"


// ����������ȡ���������(x,y)��ʼ����w����h
void EPD_InvertRect(u8 *image, u16 x, u16 y, u16 w, u16 h)
{
	/* ����ʾ������Ͳ��������Զ�������ת������ӳ�䵽�Դ������ȡ�� */
	u16 Xd, Yd;          /* ��ʾ���� */
	u16 Xm, Ym;          /* �Դ����꣨��Paint_SetPixelһ�µ�ӳ�䣩 */
	u16 xEnd, yEnd;      /* ��ʾ����Ĳü����յ� */
	u16 dispMaxX, dispMaxY;  /* ��ʾ����ϵ�µ����Χ */
	unsigned int rowBytes;   
	unsigned int addr;   
	u8 mask; 

	/* ������ת������ʾ������÷�Χ����Paint_SetPixel��ͬ����ʾ����ϵ�� */
	if(Paint.rotate == 0 || Paint.rotate == 180)
	{
		dispMaxX = Paint.heightMemory; /* ���򳤶ȶ�Ӧ�Դ�Yά�� */
		dispMaxY = Paint.widthMemory;  /* ����߶ȶ�Ӧ�Դ�Xά�� */
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

	/* �Դ��п����ַʹ��Paint��������������������һ�� */
	if(Paint.widthByte == 0) return;
	rowBytes = Paint.widthByte;

	for(Yd = y; Yd < yEnd; Yd++)
	{
		for(Xd = x; Xd < xEnd; Xd++)
		{
			/* ����ʾ����ӳ�䵽�Դ����꣬�߼���Paint_SetPixel��rotate��Ӧ */
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

			/* ��Խ�磨�Դ�ά�ȣ� */
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
	EPD_ShowChinese(0,0,(u8 *)"���ȿ���             ",24,BLACK);
	EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
	EPD_ShowChinese(0,48,(u8 *)"ң������            ",24,BLACK);
	EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);

	EPD_Display(ImageBW);
	EPD_PartUpdate();

	while(1)
	{
		KeyNum = Key_GetNum();
	
		
		if(KeyNum == 1)	// ��һ��
		{
			flag_position ++;
			if(flag_position == 9){flag_position = 1;}
		
		}
		if(KeyNum == 2)	// ��һ��
		{
			flag_position --;
			if(flag_position == 0){flag_position = 8;}
		
		}
		if(KeyNum == 3) // ȷ��
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
					EPD_ShowChinese(0,0,(u8 *)"���ȿ���             ",24,BLACK);
					EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"ң������            ",24,BLACK);
					EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);
					EPD_InvertRect(ImageBW,0,0,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					
					break;
				
				
			}
			case 2:
			{
					Paint_Clear(WHITE);
					EPD_ShowChinese(0,0,(u8 *)"���ȿ���             ",24,BLACK);
					EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"ң������            ",24,BLACK);
					EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);
					EPD_InvertRect(ImageBW,0,24,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;				
			}			
			case 3:
			{
					Paint_Clear(WHITE);
					EPD_ShowChinese(0,0,(u8 *)"���ȿ���             ",24,BLACK);
					EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"ң������            ",24,BLACK);
					EPD_ShowString (0,72,(u8 *)"MPU6050             ",24,BLACK);
					EPD_InvertRect(ImageBW,0,48,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();	
					break;				
			}
			case 4:
			{
					Paint_Clear(WHITE);
					EPD_ShowChinese(0,0,(u8 *)"���ȿ���             ",24,BLACK);
					EPD_ShowString (0,24,(u8 *)"AD                  ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"ң������            ",24,BLACK);
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
					EPD_ShowChinese(0,24,(u8 *)"ʱ��                ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"����                ",24,BLACK);
					EPD_ShowChinese(0,72,(u8 *)"����                ",24,BLACK);
					EPD_InvertRect(ImageBW,0,0,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;					
			}
			case 6:
			{
					Paint_Clear(WHITE);
					EPD_ShowString (0,0,(u8 *)"PID                  ",24,BLACK);
					EPD_ShowChinese(0,24,(u8 *)"ʱ��                ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"����                ",24,BLACK);
					EPD_ShowChinese(0,72,(u8 *)"����                ",24,BLACK);
					EPD_InvertRect(ImageBW,0,24,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();	
					break;				
			}
			case 7:
			{
					Paint_Clear(WHITE);
					EPD_ShowString (0,0,(u8 *)"PID                  ",24,BLACK);
					EPD_ShowChinese(0,24,(u8 *)"ʱ��                ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"����                ",24,BLACK);
					EPD_ShowChinese(0,72,(u8 *)"����                ",24,BLACK);
					EPD_InvertRect(ImageBW,0,48,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;					
			}
			case 8:
			{
					Paint_Clear(WHITE);
					EPD_ShowString (0,0,(u8 *)"PID                  ",24,BLACK);
					EPD_ShowChinese(0,24,(u8 *)"ʱ��                ",24,BLACK);
					EPD_ShowChinese(0,48,(u8 *)"����                ",24,BLACK);
					EPD_ShowChinese(0,72,(u8 *)"����                ",24,BLACK);
					EPD_InvertRect(ImageBW,0,72,EPD_H,24);
					EPD_Display(ImageBW);
					EPD_PartUpdate();
					break;				
			}

		
		}
	}


}

