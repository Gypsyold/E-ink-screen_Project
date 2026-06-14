#include "bsp_key.h"


KEYS Key_Num;

void bsp_key_init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	
    RCC_AHB1PeriphClockCmd(KeyUP_GPIO_CLK | KeyDown_GPIO_CLK | KeyLeft_GPIO_CLK | KeyRight_GPIO_CLK, ENABLE);
                          	
	GPIO_InitStruct.GPIO_Pin = KeyUp_Pin | KeyDown_Pin | KeyLeft_Pin | KeyRight_Pin;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed;
	GPIO_Init(KeyUp_GPIO_Port, &GPIO_InitStruct);

}

KEYS Key_GetNum(void)
{
	KEYS Temp;
	if(Key_Num)
	{
	
		Temp = Key_Num;
		Key_Num = KEY_NONE;
		return Temp;
	}
	
	return KEY_NONE;

}


KEYS Key_GetState(void)
{
	if(GPIO_ReadInputDataBit(KeyUp_GPIO_Port,KeyUp_Pin) == 0)
	{
	
		return KEY_UP;
	}
	if(GPIO_ReadInputDataBit(KeyDown_GPIO_Port,KeyDown_Pin) == 0)
	{
	
		return KEY_DOWN;
	}
	if(GPIO_ReadInputDataBit(KeyLeft_GPIO_Port,KeyLeft_Pin) == 0)
	{
	
		return KEY_LEFT;
	}
	if(GPIO_ReadInputDataBit(KeyRight_GPIO_Port,KeyRight_Pin) == 0)
	{
	
		return KEY_RIGHT;
	}
	
	return KEY_NONE;

}


void Ket_Tick(void)
{
	static uint8_t Count;
	static KEYS CurrState,PrevState;
	
	
	Count++;
	if(Count >= 20)
	{
		Count = 0;
	
		PrevState = CurrState;
		CurrState = Key_GetState();
		
		if(CurrState == KEY_NONE && PrevState != KEY_NONE)	//只判断按键松手的时刻
		{
			Key_Num = PrevState;
		}
	}

}
