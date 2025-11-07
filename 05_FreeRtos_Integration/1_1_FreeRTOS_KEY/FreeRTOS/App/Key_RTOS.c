#include "KEY_RTOS.h"
#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>

// 按键消息队列（存储KeyMsg，替代裸机的全局标志）
static QueueHandle_t key_queue = NULL;
// 互斥锁（保护Key_Flag等共享变量）
static SemaphoreHandle_t key_mutex = NULL;
// 记录按键的状态
uint8_t Key_Flag[KEY_COUNT];


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
        uint32_t current_ms = xTaskGetTickCount() * portTICK_PERIOD_MS; // RTOS系统时间（ms）

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

// 按键模块RTOS初始化
void Key_RTOS_Init(void)
{
    // 1. 初始化GPIO（复用原有硬件配置）
    Key_GPIO_Init();
    
    // 2. 创建消息队列（缓存10条消息，避免溢出）
    key_queue = xQueueCreate(10, sizeof(KeyMsg));
    configASSERT(key_queue != NULL); // 断言确保队列创建成功
    
    // 3. 创建互斥锁（保护Key_Flag）
    key_mutex = xSemaphoreCreateMutex();
    configASSERT(key_mutex != NULL);
}



// 按键处理任务
void Key_ProcessTask(void *arg)
{
    KeyMsg msg;
    while (1)
    {
        // 等待消息（阻塞等待，超时100ms）
        if (xQueueReceive(key_queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            // 处理消息（示例）
            switch (msg.id)
            {
                case KEY1:
                    if (msg.event & KEY_EVENT_SINGLE)
                    {
                        printf("KEY1单击，时长：%ums\n", msg.press_duration);
                    }
                    else if (msg.event & KEY_EVENT_LONG)
                    {
                        printf("KEY1长按，时长：%ums\n", msg.press_duration);
                    }else if(msg.event & KEY_EVENT_DOUBLE)
					{
						printf("KEY1双击\n");
					}
                    // 其他事件...
                    break;
					
                case KEY2:
                    if (msg.event & KEY_EVENT_SINGLE)
                    {
                        printf("KEY2单击，时长：%ums\n", msg.press_duration);
                    }
                    else if (msg.event & KEY_EVENT_LONG)
                    {
                        printf("KEY2长按，时长：%ums\n", msg.press_duration);
                    }else if(msg.event & KEY_EVENT_DOUBLE)
					{
						printf("KEY2双击\n");
					}
                    // 其他事件...
                    break; 
					
                case KEY3:
                    if (msg.event & KEY_EVENT_SINGLE)
                    {
                        printf("KEY3单击，时长：%ums\n", msg.press_duration);
                    }
                    else if (msg.event & KEY_EVENT_LONG)
                    {
                        printf("KEY3长按，时长：%ums\n", msg.press_duration);
                    }else if(msg.event & KEY_EVENT_DOUBLE)
					{
						printf("KEY3双击\n");
					}
                    // 其他事件...
                    break; 
					
                case KEY4:
                    if (msg.event & KEY_EVENT_SINGLE)
                    {
                        printf("KEY4单击，时长：%ums\n", msg.press_duration);
                    }
                    else if (msg.event & KEY_EVENT_LONG)
                    {
                        printf("KEY4长按，时长：%ums\n", msg.press_duration);
                    }else if(msg.event & KEY_EVENT_DOUBLE)
					{
						printf("KEY4双击\n");
					}
                    // 其他事件...
                    break;  					
            }
        }
    }
}



void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Key_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
