
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
	
		/************************全刷清屏************************/	
//	EPD_Init();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// 绑定画布
//	Paint_Clear(WHITE);											// 清空画布为白底
//	EPD_Display_Clear();
//	
//	// 关键：把画布送到屏RAM再刷新
//	EPD_Display(ImageBW);
//	EPD_Update();

//	EPD_DeepSleep();
//	Paint_Clear(WHITE);											//清除画布缓存	
	
	/************************局刷模式************************/
	EPD_Init();
	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// 绑定画布
	Paint_Clear(WHITE);

	// 上电先全屏清白并全刷一次，去残影
	EPD_Display_Clear();
	EPD_Update();
	EPD_Clear_R26H();											// 进入局刷对比模式
	delay_ms(1000);
	


	while(1)
	{
		
		KeyNum = Key_GetNum();
		
		if(KeyNum == 1)
		{
			FlashFlag = !FlashFlag;
		}
		
		if(FlashFlag)
		{

			LED1_SetMode(1);
		
		
		}else
		{
			LED1_SetMode(0);

		}
		
		
		Paint_Clear(WHITE);
		EPD_ShowString(0, 33, (u8*)"num = ", 16, BLACK);
		EPD_ShowNum(48, 33, i++, 6, 16, BLACK);			// 6位宽，按需调整位数
		EPD_Display(ImageBW);
		EPD_PartUpdate();


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



