#include "dma.h"
#include "SPI_Init.h"
#include "stm32f10x.h"                  // Device header

//void DMA_Configuration(void) 
//{
//  DMA_InitTypeDef DMA_InitStructure;

//  // 使能DMA1时钟
//  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

//  // 配置DMA1_Channel3（SPI1_TX）
//  DMA_DeInit(DMA1_Channel3);
//  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;   // 外设地址：SPI1数据寄存器
//  DMA_InitStructure.DMA_MemoryBaseAddr = 0; 						// 内存地址暂不设置（传输时指定）
//  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; 				// 内存→外设
//  DMA_InitStructure.DMA_BufferSize = 0; 							// 传输长度暂不设置（传输时指定）
//  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  // 外设地址不递增
//  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 			// 内存地址递增
//  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 8位
//  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; 	// 8位
//  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; 					// 正常模式（单次传输）
//  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; 			// 优先级
//  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 						// 非内存到内存
//  DMA_Init(DMA1_Channel3, &DMA_InitStructure);

//  // 配置DMA中断（传输完成后通知CPU）
//  NVIC_InitTypeDef NVIC_InitStructure;
//  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);

//  // 使能SPI1的DMA发送请求
//  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
//}




//// DMA传输完成中断服务函数
//void DMA1_Channel3_IRQHandler(void) 
//{
//  if (DMA_GetITStatus(DMA1_IT_TC3) != RESET) // 传输完成中断
//	{ 
//		DMA_ClearITPendingBit(DMA1_IT_TC3); // 清除中断标志
//		DMA_Cmd(DMA1_Channel3, DISABLE); // 关闭DMA通道
//		
//		// 传输完成后操作：拉高CS
//		GPIO_SetBits(EPD_CS_GPIO_PORT, EPD_CS_GPIO_PIN); 
//	}
//}

