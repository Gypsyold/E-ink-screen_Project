
#include "delay.h"
#include "usart.h"
#include "EPD_GUI.h"
#include "timer.h"
#include "KEY.h"
#include "LED.h"
#include "menu.h"




u8 ImageBW[5624];
u16 i;
int menu_next2_data_moode;
int menu_next2_read_moode;

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

//	// �ؼ����ѻ����͵���RAM��ˢ��
//	EPD_Display(ImageBW);
//	EPD_Update();

//	EPD_DeepSleep();
//	Paint_Clear(WHITE);											//�����������	
		/************************ȫˢ����************************/	
	
//	/************************��ˢģʽ************************/
	EPD_Init();
	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// �󶨻���
	Paint_Clear(WHITE);

	// �ϵ���ȫ����ײ�ȫˢһ�Σ�ȥ��Ӱ
	EPD_Display_Clear();
	EPD_Update();
	EPD_Clear_R26H();											// �����ˢ�Ա�ģʽ
	delay_ms(1000);
	


	while(1)
	{
//		Paint_Clear(WHITE);
//		EPD_ShowPicture(0,0,EPD_H,EPD_W,gImage_zonghuamian,WHITE);	
//		EPD_ShowChinese(23,121,(u8 *)"����ģʽ",24,BLACK);
//		EPD_ShowChinese(171,121,(u8 *)"�Ķ�ģʽ",24,BLACK);		
//		
//		EPD_InvertRect(ImageBW,0,0,EPD_H/2,EPD_W);
//		EPD_Display(ImageBW);
//		EPD_PartUpdate();
	
		
		Paint_Clear(WHITE);
		menu_main();
		delay_ms(500);
		

		

		
		
		
	}
	
	
	


}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{

		Ket_Tick();
		LED_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}



