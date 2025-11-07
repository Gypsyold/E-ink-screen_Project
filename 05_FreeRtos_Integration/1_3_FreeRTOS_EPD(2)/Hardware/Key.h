#ifndef __KEY_H
#define __KEY_H
#include "stdint.h"


#define KEY_PRESSED				1		//按下
#define KEY_UNPRESSED			0		//未按下

#define KEY_TIME_DOUBLE			200
#define KEY_TIME_LONG			2000
#define KEY_TIME_REPEAT			100

#define KEY_COUNT	4
// 按键ID（支持扩展）
typedef enum 
{
    KEY1,
    KEY2,
    KEY3,
    KEY4,
    KEY_MAX
} KeyID;


//初始化按键GPIO引脚
void Key_GPIO_Init(void);

//读取电平
uint8_t Key_ReadRaw(KeyID id);

#endif
