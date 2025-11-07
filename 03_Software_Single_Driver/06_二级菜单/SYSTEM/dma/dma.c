#include "dma.h"
#include "SPI_Init.h"
#include "stm32f10x.h"                  // Device header

#include "stm32f10x.h"

// DMA句柄（全局，方便中断和传输函数访问）
DMA_InitTypeDef DMA_InitStructure;

// 传输完成标志：0=进行中，1=完成
volatile uint8_t g_epd_dma_tx_done = 1;

// 初始化DMA（绑定到SPI发送）
void EPD_DMA_Init(void) 
{
	  NVIC_InitTypeDef NVIC_InitStructure;
  // 使能DMA时钟（STM32F1的SPI1_TX通常对应DMA1_Channel3）
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

  // 配置DMA通道（SPI1_TX → DMA1_Channel3）
  DMA_DeInit(DMA1_Channel3); // 复位通道配置
  DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&SPI1->DR; // 外设地址：SPI1数据寄存器
  DMA_InitStructure.DMA_MemoryBaseAddr = 0; // 内存地址暂不设置（传输时指定）
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST; // 方向：内存→外设
  DMA_InitStructure.DMA_BufferSize = 0; // 传输长度暂不设置（传输时指定）
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址不递增
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; // 内存地址递增（数组连续传输）
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; // 8位数据
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; // 8位数据
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal; // 正常模式（单次传输后停止）
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; // 优先级
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; // 非内存到内存模式
  DMA_Init(DMA1_Channel3, &DMA_InitStructure);

  // 配置DMA中断（传输完成后通知CPU处理）

  NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel3_IRQn; // DMA1通道3中断
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级（根据需求调整）
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // 子优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; // 使能中断
  NVIC_Init(&NVIC_InitStructure);

  // 使能SPI1的DMA发送请求（关键：绑定SPI和DMA）
  SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

  // 使能DMA传输完成中断
  DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
}


// 用DMA发送数组（适用于大块显存数据）
void EPD_SendData_DMA(const uint8_t *data, uint16_t len) 
{
  // 1. 确保DMA通道处于关闭状态
  DMA_Cmd(DMA1_Channel3, DISABLE);

  // 2. 设置传输参数
  DMA1_Channel3->CMAR = (uint32_t)data; // 内存地址：数组首地址
  DMA1_Channel3->CNDTR = len; // 传输长度：数组字节数

  // 3. 切换到数据模式（DC拉高），选中屏幕（CS拉低）
  EPD_DC_Set(); // 数据模式（与原EPD_WR_DATA81一致）
  EPD_CS_Clr(); // 选中屏幕

  // 4. 清除之前的中断标志，启动DMA传输
  DMA_ClearFlag(DMA1_FLAG_TC3); // 清除传输完成标志
  g_epd_dma_tx_done = 0;
  DMA_Cmd(DMA1_Channel3, ENABLE); // 开始传输
}



// DMA1通道3中断服务函数（SPI1_TX传输完成）
void DMA1_Channel3_IRQHandler(void) 
{
  // 检查是否是传输完成中断
  if (DMA_GetITStatus(DMA1_IT_TC3) != RESET) {
    // 1. 清除中断标志
    DMA_ClearITPendingBit(DMA1_IT_TC3);

    // 2. 关闭DMA通道（防止重复传输）
    DMA_Cmd(DMA1_Channel3, DISABLE);

    // 3. 等待SPI硬件完成最后一字节发送（确保总线空闲）
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);

    // 4. 传输完成，拉高CS释放屏幕
    EPD_CS_Set();
    g_epd_dma_tx_done = 1;
  }
}

