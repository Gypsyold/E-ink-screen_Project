#include "stm32f10x.h"                  // Device header
#include "Delay.h"


uint8_t Key_Num;


/**
  * ��    ����������ʼ��
  * ��    ������
  * �� �� ֵ����
  */
void Key_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/*����ʱ��*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//����GPIOB��ʱ��
	
	/*GPIO��ʼ��*/

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//��PB1��PB11���ų�ʼ��Ϊ��������
}

/**
  * ��    ����������ȡ����
  * ��    ������
  * �� �� ֵ�����°����ļ���ֵ����Χ��0~2������0����û�а�������
  * ע������˺���������ʽ��������������ס����ʱ�������Ῠס��ֱ����������
  */
uint8_t Key_GetNum(void)
{
	uint8_t Temp;
	if(Key_Num)
	{
	
		Temp = Key_Num;
		Key_Num = 0;
		return Temp;
	}
	
	return 0;

}


uint8_t Key_GetState(void)
{
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_1) == 0)
	{
	
		return 1;
	}
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11) == 0)
	{
	
		return 2;
	}
	
	return 0;

}


void Ket_Tick(void)
{
	static uint8_t Count;
	static uint8_t CurrState,PrevState;
	
	
	Count++;
	if(Count >= 20)
	{
		Count = 0;
	
		PrevState = CurrState;
		CurrState = Key_GetState();
		
		if(CurrState == 0 && PrevState != 0)	//ֻ�жϰ������ֵ�ʱ��
		{
			Key_Num = PrevState;
		}
	}

}

