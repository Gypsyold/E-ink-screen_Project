
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
		/************************全刷清屏************************/	
	
		/************************显示汉字************************/		
//	EPD_Init();
//	Paint_NewImage(ImageBW,EPD_W,EPD_H,0,WHITE);		//创建画布
//	Paint_Clear(WHITE);	
//	EPD_Display_Clear();
//	//EPD_ShowChinese(0,0,"风扇控制",24,BLACK);	
//	menu2 = menu1();
//	EPD_Display(ImageBW);

//	EPD_Update();
//	EPD_DeepSleep();
//	Paint_Clear(WHITE);  		//清除画布缓存
//	delay_ms(1000);


	/************************快刷************************/
//	EPD_FastInit();
//	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// 绑定画布
//	Paint_Clear(WHITE);											// 清空画布为白底
//	EPD_Display_Clear();
//	
//	// 绘制到画布

//	EPD_ShowChinese(44,136, "水墨屏" ,16,BLACK);		//如果是横屏的话是在最后一行，字号是16，136+16-1 = 151刚好是在最后一行
//	// 关键：把画布送到屏RAM再刷新
//	EPD_Display(ImageBW);
//	EPD_FastUpdate();

//	EPD_DeepSleep();
//	Paint_Clear(WHITE);											//清除画布缓存	

//	/************************局刷模式************************/
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



