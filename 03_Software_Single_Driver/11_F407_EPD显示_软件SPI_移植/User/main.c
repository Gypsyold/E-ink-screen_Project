#include "stm32f4xx.h"                  // Device header
#include "board.h"
#include "bsp_uart.h"
#include <stdio.h>
#include "EPD_GUI.h"
#include "Pic.h"

u8 ImageBW[5624];

int main(void)
{
	
	board_init();
	uart1_init(115200U);

	EPD_GPIOInit();
	EPD_Init();
	

	Paint_NewImage(ImageBW,EPD_W,EPD_H,0,WHITE);		//´´½¨»­²¼
	Paint_Clear(WHITE);	
	EPD_Display_Clear();		
	EPD_ShowString(36,130,"hellonih",16,BLACK);
	EPD_Display(ImageBW);
	EPD_Update();
	EPD_DeepSleep();

	while(1)
	{
		
		
	}
}

