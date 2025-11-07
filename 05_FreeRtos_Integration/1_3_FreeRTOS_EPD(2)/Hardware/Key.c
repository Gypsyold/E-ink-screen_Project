#include "Key.h"
#include "stm32f10x.h"                  // Device header


// 按键硬件配置（移植）
typedef struct 
{
    GPIO_TypeDef* port;      // GPIO端口
    uint16_t pin;            // 引脚号
    uint8_t active_level;    // 有效电平（0=低电平有效，1=高电平有效）
} KeyHWConfig;

// 硬件配置表（）
static const KeyHWConfig key_hw[KEY_MAX] = 
{
    {GPIOB, GPIO_Pin_1,  0},    // KEY1：PB1， 低电平有效
    {GPIOB, GPIO_Pin_11, 0},    // KEY2：PB11，低电平有效（按下为低）
    {GPIOB, GPIO_Pin_9,  0},    // KEY3：PB9， 低电平有效
    {GPIOB, GPIO_Pin_13, 0},    // KEY4：PB13，高电平有效（按下为高）
};



// 初始化GPIO（输入模式配置）
 void Key_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    for (int i = 0; i < KEY_MAX; i++) 
    {
        KeyID id = (KeyID)i;
        GPIO_InitStruct.GPIO_Pin = key_hw[id].pin;
        // 低电平有效→上拉输入（默认高，按下拉低）；高电平有效→下拉输入（默认低，按上拉高）
        GPIO_InitStruct.GPIO_Mode = (key_hw[id].active_level == 0) ? GPIO_Mode_IPU : GPIO_Mode_IPD;
        GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; // 输入模式无效，仅占位
        GPIO_Init(key_hw[id].port, &GPIO_InitStruct);
    }
}

// 读取电平（转换为逻辑电平）
uint8_t Key_ReadRaw(KeyID id) 
{
    if (id >= KEY_MAX) return 0; // 越界保护
    uint8_t hw_level = GPIO_ReadInputDataBit(key_hw[id].port, key_hw[id].pin);
    return (hw_level == key_hw[id].active_level) ? KEY_PRESSED : KEY_UNPRESSED;
}
