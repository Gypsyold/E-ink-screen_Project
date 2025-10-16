#include "SPI_Init.h"



void SPI_Configuration(void) 
{
  SPI_InitTypeDef SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  // ʹ��SPI1��GPIOAʱ��
  RCC_APB2PeriphClockCmd(EPD_SCL_SPI_CLK | EPD_SCL_GPIO_CLK, ENABLE);

  // ����SCK��PA5����MOSI��PA7��Ϊ�����������
  GPIO_InitStructure.GPIO_Pin = EPD_SCL_GPIO_PIN | EPD_SDA_GPIO_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // �������죨SPI���ܣ�
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  // SPI1����
  SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx; // ������
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master; // ����ģʽ
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; // 8λ����
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; // ʱ�Ӽ��ԣ����п���ʱ�͵�ƽ
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; // ʱ����λ����һ�����ز���
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft; // �������Ƭѡ
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8; // ��Ƶ��72MHz/8=9MHz�������Ļ���Ƶ�ʣ�
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; // ��λ��ǰ
  SPI_InitStructure.SPI_CRCPolynomial = 7; // ��ʹ��CRC����������
  SPI_Init(SPI1, &SPI_InitStructure);

  // ʹ��SPI1
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
	
	
	// ��ʼ״̬��CS���ߣ�δѡ�У���DCĬ�ϵͣ�ָ��ģʽ��
	EPD_CS_Set();
	EPD_DC_Clr();
	
}

void EPD_WR_Bus(u8 dat)
{

	EPD_CS_Clr();
	// �ȴ�SPI���ͻ�����Ϊ��
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
	// �������ݣ�Ӳ���Զ�����SCK��MOSI��ʱ��
	SPI_I2S_SendData(SPI1, dat);
	// �ȴ�������ɣ�ȷ�����ݷ������������CS��
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






