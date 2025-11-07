#include "stm32f4xx.h"                  // Device header
#include "board.h"
#include "stdio.h"
#include "freertos.h"
#include "task.h"
#include "bsp_uart.h"
#include "sys_timer.h"
#include "KEY_RTOS.h"

TaskHandle_t keyRTOSHandler;
int main(void)
{

	
	board_init();
	uart1_init(115200U);
	sys_Timer_Init();
	Key_RTOS_Init();
	printf("start\r\n");
	
	xTaskCreate(Key_ProcessTask,"KEYPROCESS",128,NULL,2,&keyRTOSHandler);
	vTaskStartScheduler();
	
	
	while(1)
	{

		
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
