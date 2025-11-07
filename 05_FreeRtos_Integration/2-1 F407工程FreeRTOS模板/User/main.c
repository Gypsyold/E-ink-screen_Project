#include "stm32f4xx.h"                  // Device header
#include "board.h"
#include "stdio.h"
#include "freertos.h"
#include "task.h"

//动态创建
//TaskHandle_t myTaskHandler;
//void myTask(void *arg)
//{

//	while(1)
//	{
//		GPIO_ResetBits(GPIOB,GPIO_Pin_2);
//		
//		vTaskDelay(1000);
//		GPIO_SetBits(GPIOB,GPIO_Pin_2);
//		vTaskDelay(1000);
//	
//	}
//}


//静态创建
StaticTask_t	myTaskTCB;
StackType_t		myTaskStack[128];
void myTask(void *arg)
{

	while(1)
	{
		GPIO_ResetBits(GPIOB,GPIO_Pin_2);
		
		vTaskDelay(1000);
		GPIO_SetBits(GPIOB,GPIO_Pin_2);
		vTaskDelay(1000);
	
	}
}




void LED_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_Init(GPIOB, &GPIO_InitStructure);

}



int main(void)
{

	
	board_init();
	LED_init();

	//xTaskCreate(myTask,"myTask",128,NULL,2,&myTaskHandler);				//动态创建
	
	xTaskCreateStatic(myTask,"myTask",128,NULL,2,myTaskStack,&myTaskTCB);	//静态创建
	
	
	vTaskStartScheduler();
	
	
	while(1)
	{
		GPIO_SetBits(GPIOB,GPIO_Pin_2);
		
	}
	

}


StaticTask_t	IdleTaskTCB;
StackType_t		IdleTaskStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, 
									StackType_t **ppxIdleTaskStackBuffer,
									uint32_t *pulIdleTaskStackSize )
{
	*ppxIdleTaskTCBBuffer = &IdleTaskTCB;
	*ppxIdleTaskStackBuffer = IdleTaskStack;
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;

}
