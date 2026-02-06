#include "bsp_key.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_rcc.h"

/************************ 硬件配置表（与你原配置完全一致，仅改此处适配硬件） ************************/
// 按键硬件结构体：端口+引脚+有效电平（0=低电平有效，1=高电平有效）
typedef struct 
{
    GPIO_TypeDef* port;      // GPIO端口（如GPIOC）
    uint16_t pin;            // GPIO引脚（如GPIO_Pin_0）
    uint8_t active_level;    // 有效电平（0=低电平有效，你原配置全为0）
} KeyHWConfig;

// 你的实际硬件配置：KEY1-KEY3=PC0-2低电平有效，KEY4=PC3低电平有效（原注释笔误修正）
static const KeyHWConfig key_hw[KEY_MAX] = 
{
    {GPIOC, GPIO_Pin_0, 0},    // KEY1：PC0，低电平有效
    {GPIOC, GPIO_Pin_1, 0},    // KEY2：PC1，低电平有效
    {GPIOC, GPIO_Pin_2, 0},    // KEY3：PC2，低电平有效
    {GPIOC, GPIO_Pin_3, 0},    // KEY4：PC3，低电平有效（原注释高电平，配置表统一为0）
};

/************************ 静态全局变量（底层内部使用，不对外暴露，防止误修改） ************************/
static uint8_t Key_Flag[KEY_COUNT] = {0}; // 按键事件标志位，存储各按键触发的事件
static uint8_t CurrState[KEY_COUNT] = {0}, PrevState[KEY_COUNT] = {0}; // 按键当前/上一次电平状态
static KeyState S[KEY_COUNT] = {KEY_STATE_IDLE}; // 每个按键独立状态机，初始为空闲
static uint16_t Time[KEY_COUNT] = {0}; // 长按/双击倒计时器
static uint32_t press_start[KEY_COUNT] = {0}; // 记录每个按键按下的开始时间（计算时长）
static uint8_t Count = 0; // 扫描计数（累计20ms触发一次完整扫描，与你原逻辑一致）

/************************ 静态内部函数（底层内部使用，不对外暴露，仅电平读取） ************************/
// 读取按键原始电平，并转换为逻辑电平（硬件电平→按下/未按下）
// 参数：id-按键ID；返回：KEY_PRESSED/KEY_UNPRESSED
static uint8_t Key_ReadRaw(KeyID id) 
{
    if (id >= KEY_MAX) return KEY_UNPRESSED; // 越界保护，非法ID返回未按下
    // 读取硬件原始电平
    uint8_t hw_level = GPIO_ReadInputDataBit(key_hw[id].port, key_hw[id].pin);
    // 转换为逻辑电平：硬件电平==有效电平 → 按键按下
    return (hw_level == key_hw[id].active_level) ? KEY_PRESSED : KEY_UNPRESSED;
}

/************************ 对外API实现（纯硬件，无RTOS，与你原逻辑一致） ************************/
// 按键GPIO初始化：修复原代码冗余的GPIO_OType配置（输入模式无需配置输出类型）
void BSP_Key_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    // 使能GPIOC时钟（你的按键都在GPIOC，仅需使能一次）
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    // 遍历所有按键，配置GPIO输入模式
    for (KeyID i = KEY1; i < KEY_MAX; i++) 
    {
        GPIO_InitStruct.GPIO_Pin = key_hw[i].pin;
        GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN; // 纯输入模式，无需配置输出
        // 低电平有效→上拉输入（默认高电平，按下拉低）；高电平有效→下拉输入
        GPIO_InitStruct.GPIO_PuPd = (key_hw[i].active_level == 0) ? GPIO_PuPd_UP : GPIO_PuPd_DOWN;
        GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed; // 输入模式无效，仅占位，不影响功能
        GPIO_Init(key_hw[i].port, &GPIO_InitStruct);
    }
}

// 按键扫描核心函数：20ms调用一次（建议放在TIM2中断，与你原工程一致）
// 功能：电平扫描→状态机解析→更新事件标志位，无RTOS依赖，裸机/RTOS均可调用
void BSP_Key_Tick(void)
{
    // 第一步：所有按键的长按/双击倒计时器自减（非阻塞）
    for (KeyID i = KEY1; i < KEY_MAX; i++)
    {
        if (Time[i] > 0) Time[i]--;
    }

    // 第二步：累计20ms触发一次完整扫描（与你原逻辑一致，Count++到20清零）
    Count++;
    if (Count >= 20) 
    {
        Count = 0;
        // 标准库获取系统ms时间（无RTOS也能用，替代xTaskGetTickCount）
        uint32_t current_ms = SysTick->VAL / (SystemCoreClock / 1000 / 8);

        // 第三步：遍历所有按键，逐一键处理电平+状态机
        for (KeyID i = KEY1; i < KEY_MAX; i++) 
        {
            PrevState[i] = CurrState[i]; // 保存上一次电平状态
            CurrState[i] = Key_ReadRaw(i); // 读取当前电平状态

            // 1. 处理持续按住事件（HOLD）：实时更新，按下置1，松开置0
            if (CurrState[i] == KEY_PRESSED)
            {
                Key_Flag[i] |= KEY_EVENT_HOLD;
            }
            else
            {
                Key_Flag[i] &= ~KEY_EVENT_HOLD;
            }

            // 2. 处理按下瞬间（DOWN）：仅电平从0→1时触发一次
            if (CurrState[i] == KEY_PRESSED && PrevState[i] == KEY_UNPRESSED)
            {
//                press_start[i] = current_ms; 		// 记录按下开始时间，用于计算时长
//                Key_Flag[i] |= KEY_EVENT_DOWN; 	// 置DOWN事件标志
				press_start[i] = current_ms; 	// 记录按下开始时间，用于计算时长
				(void)press_start[i];        	// 新增：伪读取，消除未使用警告
				Key_Flag[i] |= KEY_EVENT_DOWN; 	// 置DOWN事件标志
            }

            // 3. 处理松开瞬间（UP）：仅电平从1→0时触发一次
            if (CurrState[i] == KEY_UNPRESSED && PrevState[i] == KEY_PRESSED)
            {
                Key_Flag[i] |= KEY_EVENT_UP; // 置UP事件标志
            }

            // 4. 核心：状态机解析→单击/双击/长按/重复事件（与你原逻辑完全一致）
            switch (S[i])
            {
                case KEY_STATE_IDLE: // 空闲状态，检测到按下则进入等待状态
                    if (CurrState[i] == KEY_PRESSED)
                    {
                        Time[i] = KEY_TIME_LONG; // 加载长按判定时间
                        S[i] = KEY_STATE_WAIT_HOLD_OR_UP;
                    }
                    break;

                case KEY_STATE_WAIT_HOLD_OR_UP: // 等待长按或松开
                    if (CurrState[i] == KEY_UNPRESSED) // 提前松开，进入等待双击
                    {
                        Time[i] = KEY_TIME_DOUBLE; // 加载双击判定时间
                        S[i] = KEY_STATE_WAIT_DOUBLE;
                    }
                    else if (Time[i] == 0) // 超时未松开，触发长按
                    {
                        Key_Flag[i] |= KEY_EVENT_LONG; // 置LONG事件标志
                        Time[i] = KEY_TIME_REPEAT; // 加载长按重复触发时间
                        S[i] = KEY_STATE_LONG_REPEAT;
                    }
                    break;

                case KEY_STATE_WAIT_DOUBLE: // 等待双击
                    if (CurrState[i] == KEY_PRESSED) // 双击超时前再次按下，触发双击
                    {
                        Key_Flag[i] |= KEY_EVENT_DOUBLE; // 置DOUBLE事件标志
                        S[i] = KEY_STATE_WAIT_DOUBLE_UP;
                    }
                    else if (Time[i] == 0) // 双击超时未按下，触发单击
                    {
                        Key_Flag[i] |= KEY_EVENT_SINGLE; // 置SINGLE事件标志
                        S[i] = KEY_STATE_IDLE; // 回到空闲状态
                    }
                    break;

                case KEY_STATE_WAIT_DOUBLE_UP: // 等待双击后松开
                    if (CurrState[i] == KEY_UNPRESSED) // 双击后松开，回到空闲
                    {
                        S[i] = KEY_STATE_IDLE;
                    }
                    break;

                case KEY_STATE_LONG_REPEAT: // 长按重复触发状态
                    if (CurrState[i] == KEY_UNPRESSED) // 长按后松开，回到空闲
                    {
                        S[i] = KEY_STATE_IDLE;
                    }
                    else if (Time[i] == 0) // 重复触发超时，置REPEAT标志
                    {
                        Key_Flag[i] |= KEY_EVENT_REPEAT; // 置REPEAT事件标志
                        Time[i] = KEY_TIME_REPEAT; // 重新加载重复触发时间
                    }
                    break;

                default: // 异常状态，强制回到空闲
                    S[i] = KEY_STATE_IDLE;
                    break;
            }
        }
    }
}

// 检查按键事件：对外提供的事件查询接口，一次性事件自动清除，持续事件不清除
// 参数：id-按键ID，event-要检查的事件（可组合，如KEY_EVENT_SINGLE|KEY_EVENT_DOUBLE）
// 返回：1-事件触发，0-事件未触发
uint8_t BSP_Key_Check(KeyID id, uint8_t event)
{
    // 越界保护：非法按键ID或无效事件，直接返回0
    if (id >= KEY_MAX || (event & ~(0x7F)))
    {
        return 0;
    }
    uint8_t ret = 0;
    // 检测事件标志位是否置1
    if (Key_Flag[id] & event)
    {
        ret = 1;
        // 非持续事件（除了HOLD），触发后自动清除标志位，防止重复触发
        if (event != KEY_EVENT_HOLD)
        {
            Key_Flag[id] &= ~event;
        }
    }
    return ret;
}

