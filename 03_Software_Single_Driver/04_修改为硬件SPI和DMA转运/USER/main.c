
#include "delay.h"
#include "usart.h"
#include "EPD_GUI.h"
#include "Pic.h"
#include "timer.h"
#include "KEY.h"
#include "LED.h"
#include "menu.h"




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
	EPD_Clear_R26H();											// �����ˢ�Ա�ģʽ
	delay_ms(1000);
	


	while(1)
	{
		

	
		Paint_Clear(WHITE);
		menu2 = menu1();
//		if(menu2 == 1){  }
//		if(menu2 == 2){  }			
//		if(menu2 == 3){  }
//		if(menu2 == 4){  }
//		if(menu2 == 5){  }
//		if(menu2 == 6){  }
//		if(menu2 == 7){  }
//		if(menu2 == 8){  }
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



