
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "EPD_GUI.h"
#include "Pic.h"
#include "timer.h"
#include "KEY.h"
#include "LED.h"
#include "menu.h"

volatile unsigned long g_ms_ticks = 0; // ms tick for FPS measurement
u8 ImageBW[5624];
u16 i;
int menu2;

int main()
{
	delay_init();
	uart_init(115200);
	SPI_Configuration();
	EPD_GPIOInit();
	Key_Init();
	LED_Init();
	Timer_Init();
	
		/************************ȫˢ����************************/	
//	EPD_Init();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
//	Paint_Clear(WHITE);											// ��ջ���Ϊ�׵�
//	EPD_Display_Clear();
//	
//	// �ؼ����ѻ����͵���RAM��ˢ��
//	EPD_Display(ImageBW);
//	EPD_Update();

//	EPD_DeepSleep();
//	Paint_Clear(WHITE);											//�����������	
		/************************ȫˢ����************************/	
	
		/************************��ʾ����************************/		
//	EPD_Init();
//	Paint_NewImage(ImageBW,EPD_W,EPD_H,0,WHITE);		//��������
//	Paint_Clear(WHITE);	
//	EPD_Display_Clear();
//	//EPD_ShowChinese(0,0,"���ȿ���",24,BLACK);	
//	menu2 = menu1();
//	EPD_Display(ImageBW);

//	EPD_Update();
//	EPD_DeepSleep();
//	Paint_Clear(WHITE);  		//�����������
//	delay_ms(1000);


	/************************��ˢ************************/
//	EPD_FastInit();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
//	Paint_Clear(WHITE);											// ��ջ���Ϊ�׵�
//	EPD_Display_Clear();
//	
//	// ���Ƶ�����

//	EPD_ShowChinese(44,136, "ˮī��" ,16,BLACK);		//����Ǻ����Ļ��������һ�У��ֺ���16��136+16-1 = 151�պ��������һ��
//	// �ؼ����ѻ����͵���RAM��ˢ��
//	EPD_Display(ImageBW);
//	EPD_FastUpdate();

//	EPD_DeepSleep();
//	Paint_Clear(WHITE);											//�����������	

//	/************************��ˢģʽ************************/
	EPD_Init();
	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
	Paint_Clear(WHITE);

	// �ϵ���ȫ����ײ�ȫˢһ�Σ�ȥ��Ӱ
	EPD_Display_Clear();
	EPD_Update();
	EPD_Clear_R26H();										// �����ˢ�Ա�ģʽ
	delay_ms(1000);
	


	while(1)
	{
		static u8 to_black = 0;
		/* Alternate full white and full black each frame */
		memset(ImageBW, to_black ? 0x00 : 0xFF, sizeof(ImageBW));
		to_black = !to_black;

		unsigned long t0 = g_ms_ticks;
		EPD_Display(ImageBW);
		EPD_PartUpdate();
		unsigned long t1 = g_ms_ticks;
		unsigned long dt = (t1 >= t0) ? (t1 - t0) : (0xFFFFFFFFUL - t0 + t1 + 1UL);
		if (dt > 0)
		{
			float fps = 1000.0f / (float)dt;
			//ʾ��һ֡�ӿ�ʼ��ͼ��DMA����һ��ˢ��������ĵĺ�����
			printf("Loop refresh: %lu ms, FPS=%.2f\r\n", dt, fps);
			
			//FPS:ÿ��ˢ�¶��ٻ���
		}
	}
	
	
	


}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		g_ms_ticks++;
		
		Ket_Tick();
		LED_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}



