#include "SPI_Init.h"



void SPI_Configuration(void) 
{
  SPI_InitTypeDef SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  // 使能SPI1和GPIOA时钟
  RCC_APB2PeriphClockCmd(EPD_SCL_SPI_CLK | EPD_SCL_GPIO_CLK, ENABLE);

  // 配置SCK（PA5）和MOSI（PA7）为复用推挽输出
  GPIO_InitStructure.GPIO_Pin = EPD_SCL_GPIO_PIN | EPD_SDA_GPIO_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽（SPI功能）
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // SPI1配置
  SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx; // 仅发送
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master; // 主机模式
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; // 8位数据
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; // 时钟极性：空闲空闲时低电平
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; // 时钟相位：第一个边沿采样
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; // 软件控制片选
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // 分频（72MHz/8=9MHz，需≤屏幕最大频率）
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; // 高位在前
  SPI_InitStructure.SPI_CRCPolynomial = 7; // 不使用CRC，随意设置
  SPI_Init(SPI1, &SPI_InitStructure);

  // 使能SPI1
  SPI_Cmd(SPI1, ENABLE);
}









void EPD_GPIOInit(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(EPD_RES_GPIO_CLK|EPD_DC_GPIO_CLK|EPD_CS_GPIO_CLK|EPD_BUSY_GPIO_CLK,ENABLE);
	
	
	GPIO_InitStructure.GPIO_Pin=EPD_RES_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(EPD_RES_GPIO_PORT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin=EPD_DC_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(EPD_DC_GPIO_PORT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin=EPD_CS_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(EPD_CS_GPIO_PORT,&GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin=EPD_BUSY_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode =GPIO_Mode_IPU;
	GPIO_Init(EPD_BUSY_GPIO_PORT,&GPIO_InitStructure);
	
	
	// 初始状态：CS拉高（未选中），DC默认低（指令模式）
	EPD_CS_Set();
	EPD_DC_Clr();
	
}

void EPD_WR_Bus(u8 dat)
{

	EPD_CS_Clr();
	// 等待SPI发送缓冲区为空
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	// 发送数据（硬件自动处理SCK和MOSI的时序）
	SPI_I2S_SendData(SPI1, dat);
	// 等待发送完成（确保数据发送完毕再拉高CS）
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET);
	EPD_CS_Set();	
}



void EPD_WR_REG(u8 reg)
{
	EPD_DC_Clr();
	EPD_WR_Bus(reg);
	EPD_DC_Set();
}
void EPD_WR_DATA8(u8 dat)
{
	EPD_DC_Set();
	EPD_WR_Bus(dat);
	EPD_DC_Set();
}






