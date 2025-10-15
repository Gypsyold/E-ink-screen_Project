
#include "delay.h"
#include "usart.h"
#include "EPD_GUI.h"
#include "Pic.h"
#include "timer.h"
#include "KEY.h"
#include "LED.h"

uint8_t KeyNum;
uint8_t FlashFlag;
u8 ImageBW[5624];
u16 i;

int main()
{
	delay_init();
	uart_init(115200);
	EPD_GPIOInit();
	Key_Init();
	LED_Init();
	Timer_Init();
	
		/************************ȫˢ����************************/	
	EPD_Init();
	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
	Paint_Clear(WHITE);											// ��ջ���Ϊ�׵�
	EPD_Display_Clear();
	
	// �ؼ����ѻ����͵���RAM��ˢ��
	EPD_Display(ImageBW);
	EPD_Update();

	EPD_DeepSleep();
	Paint_Clear(WHITE);											//�����������	
		/************************ȫˢ����************************/	
	
		/************************��ʾ����************************/		
	EPD_Init();
	Paint_NewImage(ImageBW,EPD_W,EPD_H,0,WHITE);		//��������
	Paint_Clear(WHITE);	
	EPD_Display_Clear();		
	EPD_ShowChinese(0,0,"�о�԰",12,BLACK);
	EPD_ShowChinese(0,20,"�о�԰",16,BLACK);
	EPD_ShowChinese(0,40,"�о�԰",24,BLACK);
	EPD_ShowChinese(0,80,"�о�԰",32,BLACK);
	EPD_Display(ImageBW);
	EPD_Update();
	EPD_DeepSleep();
	Paint_Clear(WHITE);  		//�����������
	delay_ms(1000);


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
//	EPD_Init();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
//	Paint_Clear(WHITE);

//	// �ϵ���ȫ����ײ�ȫˢһ�Σ�ȥ��Ӱ
//	EPD_Display_Clear();
//	EPD_Update();
//	EPD_Clear_R26H();											// �����ˢ�Ա�ģʽ
//	delay_ms(1000);
	


	while(1)
	{
		
//		KeyNum = Key_GetNum();
//		
//		if(KeyNum == 1)
//		{
//			FlashFlag = !FlashFlag;
//		}
//		
//		if(FlashFlag)
//		{

//			LED1_SetMode(1);
//		
//		
//		}else
//		{
//			LED1_SetMode(0);

//		}
//		
//		
//		Paint_Clear(WHITE);
//		EPD_ShowString(0, 33, (u8*)"num = ", 16, BLACK);
//		EPD_ShowNum(48, 33, i++, 6, 16, BLACK);			// 6λ���������λ��
//		EPD_Display(ImageBW);
//		EPD_PartUpdate();


	}
	
	
	


}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{

		Ket_Tick();
//		LED_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}



