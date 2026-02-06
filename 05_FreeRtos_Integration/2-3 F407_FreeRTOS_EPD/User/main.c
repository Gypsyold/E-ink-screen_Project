#include "stm32f4xx.h"                  // Device header
#include "board.h"
#include "stdio.h"
#include "freertos.h"
#include "task.h"
#include "bsp_uart.h"
#include "sys_timer.h"
#include "KEY_RTOS.h"
#include "EPD_GUI.h"
#include "Display.h"
#include "Screen_state.h"
#include "SPI_W25Q128.h"

TaskHandle_t keyRTOSHandler;
TaskHandle_t menuRTOSHandler;
int main(void)
{

	
	board_init();
	uart1_init(115200U);
	sys_Timer_Init();
	Key_RTOS_Init();
	bsp_spi_flash_init();		// 初始化flash的SPI
	bsp_spi_dma_tx_init();		// 初始化SPI传输的DMA
	EPD_GPIOInit();
	EPD_Init();
	ScreenState_Init();
	
	
	printf("start\r\n");
	
	// 创建任务
    xTaskCreate(Key_ProcessTask, "KEYPROCESS", 256, NULL, 3, &keyRTOSHandler);
    xTaskCreate(menu_ProcessTask, "MENUPROCESS", 512, NULL, 2, &menuRTOSHandler);
	
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
