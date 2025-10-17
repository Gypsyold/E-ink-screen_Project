#ifndef _dma_h_
#define _dma_h_


#include <stdint.h>

// DMA 初始化（绑定 SPI1 Tx 到 DMA1_Channel3）
void EPD_DMA_Init(void);

// 使用 DMA 发送大块数据到 SPI（图像缓冲区）
void EPD_SendData_DMA(const uint8_t *data, uint16_t len);

// 传输完成标志（由中断置位），0=进行中，1=完成
extern volatile uint8_t g_epd_dma_tx_done;

#endif

