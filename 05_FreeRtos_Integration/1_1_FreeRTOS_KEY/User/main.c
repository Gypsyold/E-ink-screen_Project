#include "stm32f10x.h"                  // Device header
//#include "Delay.h"
//#include "OLED.h"
#include "Serial.h"
#include "timer.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Key_RTOS.h"




////动态创建任务
//TaskHandle_t myTask1Handler;
//void myTask1(void *arg)
//{

//	while(1)
//	{
//		printf("myTask1_Running!!\n");
//		vTaskDelay(500);
//		
//	
//	}
//}


////静态创建任务
//StackType_t myTask2Stack[128];
//StaticTask_t myTaskTCB;
//void myTask2(void *arg)
//{

//	while(1)
//	{
//		GPIO_ResetBits(GPIOC,GPIO_Pin_13);
//		vTaskDelay(1000);
//		GPIO_SetBits(GPIOC,GPIO_Pin_13);
//		vTaskDelay(1000);
//		
//	
//	}
//}

TaskHandle_t keyRTOSHandler;
int main(void)
{
	Timer_Init();
	Serial_Init();
	Key_RTOS_Init();
	printf("start\n");
	
//	//动态创建任务
//	xTaskCreate(myTask1,"myTask1",128,NULL,2,&myTask1Handler);
//	
//	
//	//静态创建任务
//	xTaskCreateStatic(myTask2,"myTask2",128,NULL,2,myTask2Stack,&myTaskTCB);
	
	xTaskCreate(Key_ProcessTask,"KEYPROCESS",256,NULL,2,&keyRTOSHandler);
	
	
	//开启任务调度
	vTaskStartScheduler();
	
	
	while(1)
	{
	
	
	}

}





//StaticTask_t	IdleTaskTCB;
//StackType_t		IdleTaskStack[configMINIMAL_STACK_SIZE];

////静态创建以提供空闲任务所需的内存
//void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, 
//									StackType_t **ppxIdleTaskStackBuffer, 
//									uint32_t *pulIdleTaskStackSize )
//{
//	*ppxIdleTaskTCBBuffer = &IdleTaskTCB;
//	*ppxIdleTaskStackBuffer = IdleTaskStack;
//	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;


//}













