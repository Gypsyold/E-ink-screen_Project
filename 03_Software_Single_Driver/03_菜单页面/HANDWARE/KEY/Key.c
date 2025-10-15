#include "stm32f10x.h"                  // Device header
#include "Delay.h"


uint8_t Key_Num;


/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */
void Key_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	/*开启时钟*/
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//开启GPIOB的时钟
	
	/*GPIO初始化*/

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);						//将PB1和PB11引脚初始化为上拉输入
}

/**
  * 函    数：按键获取键码
  * 参    数：无
  * 返 回 值：按下按键的键码值，范围：0~2，返回0代表没有按键按下
  * 注意事项：此函数是阻塞式操作，当按键按住不放时，函数会卡住，直到按键松手
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
		
		if(CurrState == 0 && PrevState != 0)	//只判断按键松手的时刻
		{
			Key_Num = PrevState;
		}
	}

}

