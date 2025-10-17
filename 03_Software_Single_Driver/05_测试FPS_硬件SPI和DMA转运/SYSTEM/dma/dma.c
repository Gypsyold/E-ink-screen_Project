#include "dma.h"
#include "SPI_Init.h"
#include "stm32f10x.h"                  // Device header

#include "stm32f10x.h"

// DMA�����ȫ�֣������жϺʹ��亯�����ʣ�
DMA_InitTypeDef DMA_InitStructure;

// ������ɱ�־��0=�����У�1=���
volatile uint8_t g_epd_dma_tx_done = 1;

// ��ʼ��DMA���󶨵�SPI���ͣ�
void EPD_DMA_Init(void) 
{
	  NVIC_InitTypeDef NVIC_InitStructure;
  // ʹ��DMAʱ�ӣ�STM32F1��SPI1_TXͨ����ӦDMA1_Channel3��
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // ����DMAͨ����SPI1_TX �� DMA1_Channel3��
  DMA_DeInit(DMA1_Channel3); // ��λͨ������
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR; // �����ַ��SPI1���ݼĴ���
  DMA_InitStructure.DMA_MemoryBaseAddr = 0; // �ڴ��ַ�ݲ����ã�����ʱָ����
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // �����ڴ������
  DMA_InitStructure.DMA_BufferSize = 0; // ���䳤���ݲ����ã�����ʱָ����
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // �����ַ������
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // �ڴ��ַ�����������������䣩
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 8λ����
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; // 8λ����
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // ����ģʽ�����δ����ֹͣ��
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; // ���ȼ�
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // ���ڴ浽�ڴ�ģʽ
  DMA_Init(DMA1_Channel3, &DMA_InitStructure);

  // ����DMA�жϣ�������ɺ�֪ͨCPU����

  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn; // DMA1ͨ��3�ж�
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // ��ռ���ȼ����������������
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // �����ȼ�
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; // ʹ���ж�
  NVIC_Init(&NVIC_InitStructure);

  // ʹ��SPI1��DMA�������󣨹ؼ�����SPI��DMA��
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

  // ʹ��DMA��������ж�
  DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
}


// ��DMA�������飨�����ڴ���Դ����ݣ�
void EPD_SendData_DMA(const uint8_t *data, uint16_t len) 
{
  // 1. ȷ��DMAͨ�����ڹر�״̬
  DMA_Cmd(DMA1_Channel3, DISABLE);

  // 2. ���ô������
  DMA1_Channel3->CMAR = (uint32_t)data; // �ڴ��ַ�������׵�ַ
  DMA1_Channel3->CNDTR = len; // ���䳤�ȣ������ֽ���

  // 3. �л�������ģʽ��DC���ߣ���ѡ����Ļ��CS���ͣ�
  EPD_DC_Set(); // ����ģʽ����ԭEPD_WR_DATA81һ�£�
  EPD_CS_Clr(); // ѡ����Ļ

  // 4. ���֮ǰ���жϱ�־������DMA����
  DMA_ClearFlag(DMA1_FLAG_TC3); // ���������ɱ�־
  g_epd_dma_tx_done = 0;
  DMA_Cmd(DMA1_Channel3, ENABLE); // ��ʼ����
}



// DMA1ͨ��3�жϷ�������SPI1_TX������ɣ�
void DMA1_Channel3_IRQHandler(void) 
{
  // ����Ƿ��Ǵ�������ж�
  if (DMA_GetITStatus(DMA1_IT_TC3) != RESET) {
    // 1. ����жϱ�־
    DMA_ClearITPendingBit(DMA1_IT_TC3);

    // 2. �ر�DMAͨ������ֹ�ظ����䣩
    DMA_Cmd(DMA1_Channel3, DISABLE);

    // 3. �ȴ�SPIӲ��������һ�ֽڷ��ͣ�ȷ�����߿��У�
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

    // 4. ������ɣ�����CS�ͷ���Ļ
    EPD_CS_Set();
    g_epd_dma_tx_done = 1;
  }
}

