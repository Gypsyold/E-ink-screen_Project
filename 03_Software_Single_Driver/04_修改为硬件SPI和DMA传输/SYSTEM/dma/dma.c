#include "dma.h"
#include "SPI_Init.h"
#include "stm32f10x.h"                  // Device header

//void DMA_Configuration(void) 
//{
//  DMA_InitTypeDef DMA_InitStructure;

//  // ʹ��DMA1ʱ��
//  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

//  // ����DMA1_Channel3��SPI1_TX��
//  DMA_DeInit(DMA1_Channel3);
//  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR;   // �����ַ��SPI1���ݼĴ���
//  DMA_InitStructure.DMA_MemoryBaseAddr = 0; 						// �ڴ��ַ�ݲ����ã�����ʱָ����
//  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; 				// �ڴ������
//  DMA_InitStructure.DMA_BufferSize = 0; 							// ���䳤���ݲ����ã�����ʱָ����
//  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  // �����ַ������
//  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; 			// �ڴ��ַ����
//  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 8λ
//  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; 	// 8λ
//  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; 					// ����ģʽ�����δ��䣩
//  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; 			// ���ȼ�
//  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 						// ���ڴ浽�ڴ�
//  DMA_Init(DMA1_Channel3, &DMA_InitStructure);

//  // ����DMA�жϣ�������ɺ�֪ͨCPU��
//  NVIC_InitTypeDef NVIC_InitStructure;
//  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn;
//  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
//  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
//  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
//  NVIC_Init(&NVIC_InitStructure);

//  // ʹ��SPI1��DMA��������
//  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
//}




//// DMA��������жϷ�����
//void DMA1_Channel3_IRQHandler(void) 
//{
//  if (DMA_GetITStatus(DMA1_IT_TC3) != RESET) // ��������ж�
//	{ 
//		DMA_ClearITPendingBit(DMA1_IT_TC3); // ����жϱ�־
//		DMA_Cmd(DMA1_Channel3, DISABLE); // �ر�DMAͨ��
//		
//		// ������ɺ����������CS
//		GPIO_SetBits(EPD_CS_GPIO_PORT, EPD_CS_GPIO_PIN); 
//	}
//}

