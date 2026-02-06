#include "KEY_RTOS.h"
#include "stm32f4xx.h"                  // Device header
#include <stdio.h>
#include "Screen_state.h"
#include "Display.h"  

// 按键消息队列（存储KeyMsg，替代裸机的全局标志）
static QueueHandle_t key_queue = NULL;
// 互斥锁（保护Key_Flag等共享变量）
static SemaphoreHandle_t key_mutex = NULL;
// 记录按键的状态
uint8_t Key_Flag[KEY_COUNT];
// 显示命令队列（按键任务→显示任务）
QueueHandle_t xDispCmdQueue;


// 按键硬件配置（与硬件无关，便于移植）
typedef struct 
{
    GPIO_TypeDef* port;      // GPIO端口
    uint16_t pin;            // 引脚号
    uint8_t active_level;    // 有效电平（0=低电平有效，1=高电平有效）
} KeyHWConfig;

// 硬件配置表（根据实际硬件修改）
static const KeyHWConfig key_hw[KEY_MAX] = 
{
    {GPIOC, GPIO_Pin_0, 0},    // KEY1：PC0，低电平有效
    {GPIOC, GPIO_Pin_1, 0},    // KEY2：PC1，低电平有效（按下为低）
    {GPIOC, GPIO_Pin_2, 0},    // KEY3：PC2，低电平有效
    {GPIOC, GPIO_Pin_3, 0},    // KEY4：PC3，高电平有效
};



// 初始化GPIO（输入模式配置）
 void Key_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    for (int i = 0; i < KEY_MAX; i++) 
    {
        KeyID id = (KeyID)i;
        GPIO_InitStruct.GPIO_Pin = key_hw[id].pin;
        // 低电平有效→上拉输入（默认高，按下拉低）；高电平有效→下拉输入（默认低，按上拉高）
		GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
        GPIO_InitStruct.GPIO_PuPd = (key_hw[id].active_level == 0) ? GPIO_PuPd_UP : GPIO_PuPd_DOWN;
        GPIO_InitStruct.GPIO_Speed = GPIO_High_Speed; // 输入模式无效，仅占位
        GPIO_Init(key_hw[id].port, &GPIO_InitStruct);
    }
}

// 读取电平（转换为逻辑电平）
static uint8_t Key_ReadRaw(KeyID id) 
{
    if (id >= KEY_MAX) return 0; // 越界保护
    uint8_t hw_level = GPIO_ReadInputDataBit(key_hw[id].port, key_hw[id].pin);
    return (hw_level == key_hw[id].active_level) ? KEY_PRESSED : KEY_UNPRESSED;
}



uint8_t Key_Check(uint8_t n, uint8_t Flag)
{
    uint8_t ret = 0;
    // 任务中获取互斥锁（等待10ms，避免永久阻塞）
    if (xSemaphoreTake(key_mutex, pdMS_TO_TICKS(10)) == pdTRUE)
    {
        if (Key_Flag[n] & Flag)
        {
            if (Flag != KEY_EVENT_HOLD)
            {
                Key_Flag[n] &= ~Flag; // 清除一次性标志
            }
            ret = 1;
        }
        xSemaphoreGive(key_mutex); // 释放锁
    }
    return ret;
}

static void Key_SendMsgFromISR(KeyID id, uint8_t event, uint32_t duration)
{
    KeyMsg msg = {id, event, duration};
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // 发送消息到队列（不阻塞，队列满则丢弃）
    xQueueSendFromISR(key_queue, &msg, &xHigherPriorityTaskWoken);
    // 若有更高优先级任务被唤醒，触发任务切换（带参数）
    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken); // 传入参数
    }
}


void Key_Tick(void)
{
    static uint8_t Count, i;
    static uint8_t CurrState[KEY_COUNT], PrevState[KEY_COUNT];
    static KeyState S[KEY_COUNT] = {KEY_STATE_IDLE}; // 初始化状态为空闲
    static uint16_t Time[KEY_COUNT];
    static uint32_t press_start[KEY_COUNT] = {0}; // 记录按下开始时间（计算duration）

    // 长按和双击的倒计时
    for (i = 0; i < KEY_COUNT; i++)
    {
        if (Time[i] > 0) Time[i]--;
    }

    Count++;
    if (Count >= 20) // 20ms检测一次
    {
        Count = 0;
        uint32_t current_ms = xTaskGetTickCount(); // RTOS系统时间（ms）

        for (i = 0; i < KEY_COUNT; i++)
        {
            PrevState[i] = CurrState[i];
            CurrState[i] = Key_ReadRaw((KeyID)i);
            KeyID id = (KeyID)i;

            // 1. 处理持续按住状态（HOLD标志，需互斥锁保护）
            if (xSemaphoreTakeFromISR(key_mutex, NULL) == pdTRUE) // 中断中获取锁
            {
                if (CurrState[i] == KEY_PRESSED)
                {
                    Key_Flag[i] |= KEY_EVENT_HOLD;
                }
                else
                {
                    Key_Flag[i] &= ~KEY_EVENT_HOLD;
                }
                xSemaphoreGiveFromISR(key_mutex, NULL); // 中断中释放锁
            }

            // 2. 按下瞬间（DOWN事件）
            if (CurrState[i] == KEY_PRESSED && PrevState[i] == KEY_UNPRESSED)
            {
                press_start[i] = current_ms; // 记录按下开始时间
                Key_SendMsgFromISR(id, KEY_EVENT_DOWN, 0); // 发送DOWN消息
            }

            // 3. 松开瞬间（UP事件）
            if (CurrState[i] == KEY_UNPRESSED && PrevState[i] == KEY_PRESSED)
            {
                uint32_t duration = current_ms - press_start[i]; // 计算按下时长
                Key_SendMsgFromISR(id, KEY_EVENT_UP, duration); // 发送UP消息
            }

            // 4. 状态机处理（单击/双击/长按/重复，发送对应消息）
            switch (S[i])
            {
                case KEY_STATE_IDLE:
                    if (CurrState[i] == KEY_PRESSED)
                    {
                        Time[i] = KEY_TIME_LONG;
                        S[i] = KEY_STATE_WAIT_HOLD_OR_UP;
                    }
                    break;

                case KEY_STATE_WAIT_HOLD_OR_UP:
                    if (CurrState[i] == KEY_UNPRESSED)
                    {
                        Time[i] = KEY_TIME_DOUBLE;
                        S[i] = KEY_STATE_WAIT_DOUBLE;
                    }
                    else if (Time[i] == 0)
                    {
                        // 长按事件：计算已按下时长
                        uint32_t duration = current_ms - press_start[i];
                        Key_SendMsgFromISR(id, KEY_EVENT_LONG, duration);
                        Time[i] = KEY_TIME_REPEAT;
                        S[i] = KEY_STATE_LONG_REPEAT;
                    }
                    break;

                case KEY_STATE_WAIT_DOUBLE:
                    if (CurrState[i] == KEY_PRESSED)
                    {
                        Key_SendMsgFromISR(id, KEY_EVENT_DOUBLE, 0); // 双击事件
                        S[i] = KEY_STATE_WAIT_DOUBLE_UP;
                    }
                    else if (Time[i] == 0)
                    {
                        Key_SendMsgFromISR(id, KEY_EVENT_SINGLE, 0); // 单击事件
                        S[i] = KEY_STATE_IDLE;
                    }
                    break;

                case KEY_STATE_WAIT_DOUBLE_UP:
                    if (CurrState[i] == KEY_UNPRESSED)
                    {
                        S[i] = KEY_STATE_IDLE;
                    }
                    break;

                case KEY_STATE_LONG_REPEAT:
                    if (CurrState[i] == KEY_UNPRESSED)
                    {
                        S[i] = KEY_STATE_IDLE;
                    }
                    else if (Time[i] == 0)
                    {
                        uint32_t duration = current_ms - press_start[i];
                        Key_SendMsgFromISR(id, KEY_EVENT_REPEAT, duration); // 重复事件
                        Time[i] = KEY_TIME_REPEAT;
                    }
                    break;

                default:
                    S[i] = KEY_STATE_IDLE;
                    break;
            }
        }
    }
}




// 按键模块初始化
void Key_RTOS_Init(void) 
{
    Key_GPIO_Init();
    key_queue = xQueueCreate(10, sizeof(KeyMsg));
    key_mutex = xSemaphoreCreateMutex();
    xDispCmdQueue = xQueueCreate(5, sizeof(DispCmd)); // 显示命令队列
    configASSERT(key_queue && key_mutex && xDispCmdQueue);
}





// 按键处理任务：解析事件→生成显示命令
void Key_ProcessTask(void *arg) 
{
    KeyMsg msg;
    DispCmd cmd;
    CurrentScreen curScreen;

    while (1) 
	{
        // 接收按键事件
        if (xQueueReceive(key_queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) continue;

        // 获取当前画面状态（决定按键功能）
        curScreen = ScreenState_Get();
        cmd.cmd = DISP_CMD_NONE;
        cmd.param = 0;

        // 根据“画面+按键+事件”生成命令
        switch (curScreen) 
		{
            // -------------------------- 主菜单画面 --------------------------
            case SCREEN_MAIN_MENU:
                switch (msg.id) 
				{
                    case KEY1:
                        if (msg.event & KEY_EVENT_SINGLE) 
						{
                            cmd.cmd = DISP_CMD_MENU_SEL_CHANGE;
                            cmd.param = -1; // 选中“日历模式”
                        }
                        break;
                    case KEY2:
                        if (msg.event & KEY_EVENT_SINGLE) 
						{
                            cmd.cmd = DISP_CMD_MENU_SEL_CHANGE;
                            cmd.param = 1; // 选中“阅读模式”
                        }
                        break;
                    case KEY3:
                        if (msg.event & KEY_EVENT_SINGLE) 
						{
                            // 根据当前菜单选中项进入对应模式（g_MenuSel=0→日历，1→阅读）
                            cmd.cmd = (g_MenuSel == 1) ? DISP_CMD_ENTER_CALENDAR : DISP_CMD_ENTER_READING_SEL;
                        }
                        break;
                }
                break;

            // -------------------------- 阅读-页码选择画面 --------------------------
            case SCREEN_READING_SEL:
                switch (msg.id) 
				{
                    case KEY1:
                        if (msg.event & KEY_EVENT_SINGLE) 
						{
                            cmd.cmd = DISP_CMD_READING_PAGE_CHANGE;
                            cmd.param = -1; // 上一页
                        }
                        break;
                    case KEY2:
                        if (msg.event & KEY_EVENT_SINGLE) 
						{
                            cmd.cmd = DISP_CMD_READING_PAGE_CHANGE;
                            cmd.param = 1; // 下一页
                        }
                        break;
                    case KEY3:
                        if (msg.event & KEY_EVENT_SINGLE) 
						{
                            cmd.cmd = DISP_CMD_ENTER_READING_VIEW; // 确认进入内容页
                        } else if (msg.event & KEY_EVENT_DOUBLE) 
						{
                            cmd.cmd = DISP_CMD_BACK_TO_MAIN; // 双击返回主菜单
                        }
                        break;
                }
                break;

            // -------------------------- 阅读-内容显示画面 --------------------------
            case SCREEN_READING_VIEW:
                switch (msg.id) 
				{
                    case KEY3:
                        if (msg.event & KEY_EVENT_DOUBLE) 
						{
                            cmd.cmd = DISP_CMD_BACK_TO_READING_SEL; // 双击返回页码选择
                        }
                        break;
                }
                break;

            // -------------------------- 日历模式画面 --------------------------
            case SCREEN_CALENDAR:
                switch (msg.id) 
				{
                    case KEY3:
                        if (msg.event & KEY_EVENT_DOUBLE) 
						{
                            cmd.cmd = DISP_CMD_BACK_TO_MAIN; // 双击返回主菜单
                        }
                        break;
                }
                break;
        }

        // 发送命令到显示队列
        if (cmd.cmd != DISP_CMD_NONE) 
		{
            xQueueSend(xDispCmdQueue, &cmd, 0);
        }
    }
}




