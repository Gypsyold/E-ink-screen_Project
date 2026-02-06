#ifndef BSP_KEY_H
#define BSP_KEY_H

#include "stm32f4xx.h"  // 仅依赖STM32标准库，无其他冗余依赖

/************************ 硬件按键配置（仅改此处适配硬件，与你原配置一致） ************************/
#define KEY_TIME_DOUBLE    500    // 双击判定时间(ms)
#define KEY_TIME_LONG      2000   // 长按判定时间(ms)
#define KEY_TIME_REPEAT    100    // 长按重复触发时间(ms)
#define KEY_COUNT          4      // 按键总数（KEY1-KEY4）

/************************ 按键基础枚举（纯硬件，无业务） ************************/
// 按键ID（与硬件配置表一一对应，你原配置：KEY1=PC0,KEY2=PC1,KEY3=PC2,KEY4=PC3）
typedef enum 
{
    KEY1,
    KEY2,
    KEY3,
    KEY4,
    KEY_MAX  // 边界值，用于越界保护，防止数组访问越界
} KeyID;

// 按键状态机状态（底层内部使用，对外隐藏，仅用于解析单击/双击/长按）
typedef enum 
{
    KEY_STATE_IDLE,            // 0：空闲状态（初始状态，按键未按下）
    KEY_STATE_WAIT_HOLD_OR_UP, // 1：等待长按判定或松开（首次按下后）
    KEY_STATE_WAIT_DOUBLE,     // 2：等待双击（首次松开后，未到双击超时）
    KEY_STATE_WAIT_DOUBLE_UP,  // 3：等待双击后松开（第二次按下后）
    KEY_STATE_LONG_REPEAT      // 4：长按重复触发状态（长按超时后持续触发）
} KeyState;

// 按键电平逻辑定义（底层转换用）
#define KEY_PRESSED     1   // 按键按下（逻辑电平）
#define KEY_UNPRESSED   0   // 按键未按下（逻辑电平）

/************************ 按键事件标志（独立位，支持组合，对外暴露） ************************/
// 原事件标志完全保留，一次性事件触发后自动清除，持续事件（HOLD）不清除
#define KEY_EVENT_HOLD       0x01    // 持续按住（持续事件，不自动清除）
#define KEY_EVENT_DOWN       0x02    // 按下瞬间（一次性事件）
#define KEY_EVENT_UP         0x04    // 松开瞬间（一次性事件）
#define KEY_EVENT_SINGLE     0x08    // 单击（松开后判定，一次性事件）
#define KEY_EVENT_DOUBLE     0x10    // 双击（间隔内两次按下，一次性事件）
#define KEY_EVENT_LONG       0x20    // 长按（超过阈值，一次性事件）
#define KEY_EVENT_REPEAT     0x40    // 长按重复触发（长按后持续触发，一次性事件）

/************************ 对外暴露的纯硬件按键API（无RTOS，无业务） ************************/
void BSP_Key_GPIO_Init(void);    // 按键GPIO初始化（配置输入模式+上拉/下拉）
void BSP_Key_Tick(void);         // 按键扫描核心函数（20ms调用一次，建议定时器中断调用）
uint8_t BSP_Key_Check(KeyID id, uint8_t event); // 检查按键事件，一次性事件自动清除

#endif // BSP_KEY_H

