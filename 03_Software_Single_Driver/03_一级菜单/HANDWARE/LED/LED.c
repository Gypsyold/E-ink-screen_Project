#include "stm32f10x.h"                  // Device header


uint8_t LED1_Mode;

uint16_t LED1_Count;



/**
  * ��    ����LED��ʼ��
  * ��    ������
  * �� �� ֵ����
  */
void LED_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	/*����ʱ��*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//����GPIOA��ʱ��
	
	/*GPIO��ʼ��*/

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//��PA1��PA2���ų�ʼ��Ϊ�������
	
	/*����GPIO��ʼ�����Ĭ�ϵ�ƽ*/
	GPIO_SetBits(GPIOB, GPIO_Pin_12 | GPIO_Pin_13);				//����PA1��PA2����Ϊ�ߵ�ƽ
}

/**
  * ��    ����LED1����
  * ��    ������
  * �� �� ֵ����
  */
void LED1_ON(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);		//����PA1����Ϊ�͵�ƽ
}

/**
  * ��    ����LED1�ر�
  * ��    ������
  * �� �� ֵ����
  */
void LED1_OFF(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_12);		//����PA1����Ϊ�ߵ�ƽ
}

/**
  * ��    ����LED1״̬��ת
  * ��    ������
  * �� �� ֵ����
  */
void LED1_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_12) == 0)		//��ȡ����Ĵ�����״̬�������ǰ��������͵�ƽ
	{
		GPIO_SetBits(GPIOB, GPIO_Pin_12);					//������PA1����Ϊ�ߵ�ƽ
	}
	else													//���򣬼���ǰ��������ߵ�ƽ
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_12);					//������PA1����Ϊ�͵�ƽ
	}
}

/**
  * ��    ����LED2����
  * ��    ������
  * �� �� ֵ����
  */
void LED2_ON(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_13);		//����PA2����Ϊ�͵�ƽ
}

/**
  * ��    ����LED2�ر�
  * ��    ������
  * �� �� ֵ����
  */
void LED2_OFF(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_13);		//����PA2����Ϊ�ߵ�ƽ
}

/**
  * ��    ����LED2״̬��ת
  * ��    ������
  * �� �� ֵ����
  */
void LED2_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOB, GPIO_Pin_13) == 0)		//��ȡ����Ĵ�����״̬�������ǰ��������͵�ƽ
	{                                                  
		GPIO_SetBits(GPIOB, GPIO_Pin_13);               		//������PA2����Ϊ�ߵ�ƽ
	}                                                  
	else                                               		//���򣬼���ǰ��������ߵ�ƽ
	{                                                  
		GPIO_ResetBits(GPIOB, GPIO_Pin_13);             		//������PA2����Ϊ�͵�ƽ
	}
}



void LED1_SetMode(uint8_t Mode)
{
	LED1_Mode = Mode;
	
}


void LED_Tick(void)
{
	if(LED1_Mode == 0)
	{
		LED1_OFF();
	}else if(LED1_Mode == 1)
	{
		LED1_Count++;
		
		LED1_Count %= 1000;
		
		if(LED1_Count < 500)
		{
			LED1_ON();
		
		}else
		{
			LED1_OFF();
		}	
	
	}
	
	

	
}
