//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//�о�԰����
//���̵�ַ��http://shop73023976.taobao.com
//
//  �� �� ��   : main.c
//  �� �� ��   : v2.0
//  ��    ��   : ZhaoJian
//  ��������   : 2023-11-30
//  ����޸�   : 
//  ��������   :��ʾ����(STM32F103ϵ��)
//              ˵��: 
//              ----------------------------------------------------------------
//              GND   ��Դ��
//              VCC   3.3v��Դ
//              SCL   PA0��SCLK��
//              SDA   PA1��MOSI��
//              RES   PA2
//              DC    PA3
//              CS    PA4
//              BLK   PA5
//              ----------------------------------------------------------------
// �޸���ʷ   :
// ��    ��   : 
// ��    ��   : ZhaoJian
// �޸�����   : �����ļ�
//��Ȩ���У�����ؾ���
//Copyright(C) �о�԰����2023-11-30
//All rights reserved
//******************************************************************************/

#include "delay.h"
#include "usart.h"
#include "EPD_GUI.h"
#include "Pic.h"



u8 ImageBW[5624];
//int main()
//{
//	float num=12.05;
//	u8 dat=0;
//	delay_init();
//	uart_init(115200);
//	EPD_GPIOInit();
//	/************************ȫˢ************************/
//	EPD_Init();
//	EPD_Display(gImage_1);
//	EPD_Update();
//	EPD_DeepSleep();
//	delay_ms(1000);
//  /*********************��ˢģʽ**********************/
//	EPD_FastInit();
//	EPD_Display(gImage_2);
//	EPD_FastUpdate();
//	EPD_DeepSleep();
//	delay_ms(1000);
//	/************************ȫˢ************************/
//	EPD_Init();
//	Paint_NewImage(ImageBW,EPD_W,EPD_H,0,WHITE);		//��������
//	Paint_Clear(WHITE);	
//	EPD_Display_Clear();
//	EPD_ShowPicture(40,0,216,112,gImage_3,BLACK);
//	EPD_ShowString(36,130,"2.66inch",16,BLACK);
//	EPD_ShowChinese(100,130,"����īˮ���ϵ����ʾ",16,BLACK);
//	EPD_Display(ImageBW);
//	EPD_Update();
//	EPD_DeepSleep();
//	Paint_Clear(WHITE);  		//�����������
//	delay_ms(1000);
//	/************************��ȫˢ����Ȼ�����ھ�ˢģʽ************************/
//	EPD_Init();
//	EPD_Display_Clear();
//	EPD_Update();						//���»�����ʾ
//	EPD_Clear_R26H();
//	while(1)
//	{
//		/*********************��ˢģʽ**********************/
//		EPD_ShowString(33,0,"Welcome to 2.66-inch E-paper",16,BLACK);		
//		EPD_ShowString(49,20,"with 296 x 152 resolution",16,BLACK);	
//		EPD_ShowString(68,40,"Demo-Test-2023/10/16",16,BLACK);
//		EPD_DrawLine(0,60,295,60,BLACK);
//		EPD_DrawRectangle(4,78,44,118,BLACK,0);
//		EPD_DrawRectangle(48,78,88,119,BLACK,1);
//		EPD_ShowWatch(88,74,num,4,2,48,BLACK);
//		EPD_DrawCircle(245,100,25,BLACK,0);
//		EPD_DrawCircle(265,100,25,BLACK,1);
//		EPD_ShowChinese(44,136,"֣���о�԰���ӿƼ����޹�˾",16,BLACK);
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
//}






int main()
{
//	u8 n = 0;

//	u8 str[] = "num = ";
	delay_init();
	uart_init(115200);
	EPD_GPIOInit();


	
	/************************ȫˢ************************/	
//	EPD_Init();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
//	Paint_Clear(WHITE);											// ��ջ���Ϊ�׵�
//	EPD_Display_Clear();
//	
//	// ���Ƶ�����
//	EPD_ShowString(0, 33, str, 16, BLACK);
//	
//	// �ؼ����ѻ����͵���RAM��ˢ��
//	EPD_Display(ImageBW);
//	EPD_Update();

//	EPD_DeepSleep();
//	Paint_Clear(WHITE);											//�����������



	/************************��ˢ************************/
	EPD_FastInit();
	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
	Paint_Clear(WHITE);											// ��ջ���Ϊ�׵�
	EPD_Display_Clear();
	
	// ���Ƶ�����
	//EPD_ShowString(0, 33, str, 16, BLACK);
	EPD_ShowChinese(44,136, "ˮī��" ,16,BLACK);		//����Ǻ����Ļ��������һ�У��ֺ���16��136+16-1 = 151�պ��������һ��
	// �ؼ����ѻ����͵���RAM��ˢ��
	EPD_Display(ImageBW);
	EPD_FastUpdate();

	EPD_DeepSleep();
	Paint_Clear(WHITE);											//�����������	
//	
	
	
	/*********************��ˢģʽ**********************/
	
//	EPD_Init();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
//	Paint_Clear(WHITE);

//	// �ϵ���ȫ����ײ�ȫˢһ�Σ�ȥ��Ӱ
//	EPD_Display_Clear();
//	EPD_Update();
//	EPD_Clear_R26H();											// �����ˢ�Ա�ģʽ
//	delay_ms(1000);
//	
//	Paint_Clear(WHITE);
//	EPD_ShowString(0, 0, (u8*)"helloworld", 16, BLACK);
//	EPD_Display(ImageBW);
//	EPD_PartUpdate();	
//	delay_ms(1000);
//	
//	Paint_Clear(WHITE);
//	EPD_ShowString(0, 20, (u8*)"E_PAGE", 16, BLACK);
//	EPD_Display(ImageBW);
//	EPD_PartUpdate();
//	delay_ms(1000);
//	
//	
//	while(1)
//	{
//		Paint_Clear(WHITE);
//		EPD_ShowString(0, 33, (u8*)"num = ", 16, BLACK);
//		EPD_ShowNum(48, 33, n, 6, 16, BLACK);			// 6λ���������λ��
//		EPD_Display(ImageBW);
//		EPD_PartUpdate();
//		delay_ms(500);
//		n++;
//	}



	/************************ʵ��************************/
//	EPD_FastInit();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 270, WHITE);			// �󶨻���
//	Paint_Clear(WHITE);											// ��ջ���Ϊ�׵�
//	EPD_Display_Clear();
//	
//	// ���Ƶ�����
//	//EPD_ShowString(0, 33, str, 16, BLACK);
//	EPD_ShowChinese(44,0,"ˮī��",16,BLACK);		
//	// �ؼ����ѻ����͵���RAM��ˢ��
//	EPD_Display(ImageBW);
//	EPD_FastUpdate();

//	EPD_DeepSleep();
//	Paint_Clear(WHITE);											//�����������	
	
	
	/************************ȫˢ����************************/	
//	EPD_Init();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
//	Paint_Clear(WHITE);											// ��ջ���Ϊ�׵�
//	EPD_Display_Clear();
//	
//	// ���Ƶ�����
//	//EPD_ShowString(0, 33, str, 16, BLACK);
//	
//	// �ؼ����ѻ����͵���RAM��ˢ��
//	EPD_Display(ImageBW);
//	EPD_Update();

//	EPD_DeepSleep();
//	Paint_Clear(WHITE);											//�����������	




}
