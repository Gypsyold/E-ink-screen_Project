#include "stm32f10x.h"// Device header
#include "FreeRTOS.h"
//#include "queue.h"
#include "task.h"
#include "Delay.h"
#include "Serial.h"
#include "timer.h"
#include "EPD_GUI.h"
#include "Key_RTOS.h"
#include "Display.h"
#include "Screen_state.h"
#include <stdio.h>
#include <string.h>



// 任务句柄
TaskHandle_t keyRTOSHandler;
TaskHandle_t menuRTOSHandler;

int main()
{
	Serial_Init();
	EPD_GPIOInit();
	EPD_Init();
	Timer_Init();
	Key_RTOS_Init();
	ScreenState_Init();
	
	Delay_ms(50);
	printf("start\n");
	
	
	// 创建任务
    xTaskCreate(Key_ProcessTask, "KEYPROCESS", 256, NULL, 3, &keyRTOSHandler);
    xTaskCreate(menu_ProcessTask, "MENUPROCESS", 512, NULL, 2, &menuRTOSHandler);
	
	//开启任务调度
	vTaskStartScheduler();
	
	
	while(1)
	{
	
	
	}
}

void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Key_Tick();
		

		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

















