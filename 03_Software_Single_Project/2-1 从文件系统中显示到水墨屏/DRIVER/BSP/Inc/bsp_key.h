#ifndef _bsp_key_h_
#define _bsp_key_h_


#include "stm32f4xx.h"                  // Device header
#include "board.h"

#define KeyUp_Pin 				GPIO_Pin_0
#define KeyDown_Pin 			GPIO_Pin_1
#define KeyLeft_Pin 			GPIO_Pin_2
#define KeyRight_Pin 			GPIO_Pin_3
#define SD_Detect_Pin 			GPIO_Pin_3

#define KeyUp_GPIO_Port 		GPIOC
#define KeyDown_GPIO_Port 		GPIOC
#define KeyLeft_GPIO_Port 		GPIOC
#define KeyRight_GPIO_Port 		GPIOC
#define SD_Detect_GPIO_Port 	GPIOD

#define KeyUP_GPIO_CLK			RCC_AHB1Periph_GPIOC
#define KeyDown_GPIO_CLK		RCC_AHB1Periph_GPIOC
#define KeyLeft_GPIO_CLK		RCC_AHB1Periph_GPIOC
#define KeyRight_GPIO_CLK		RCC_AHB1Periph_GPIOC
#define SD_Detect_GPIO_CLK		RCC_AHB1Periph_GPIOD




typedef enum 
{
	KEY_NONE = 0,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,	
	KEY_RIGHT,	
	
} KEYS;


void bsp_key_init(void);
KEYS Key_GetNum(void);
void Ket_Tick(void);

#endif

