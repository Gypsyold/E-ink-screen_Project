#include "timer.h"



void Timer_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 开启TIM2时钟（APB1总线）42M */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    
    TIM_InternalClockConfig(TIM2);

    
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;  // ARR=999（计数1000次）
    TIM_TimeBaseInitStructure.TIM_Prescaler = 42 - 1;  // PSC=41（42MHz/(41+1)=1kHz）
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    /* 清除更新标志位 */
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);

    /* 开启更新中断 */
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    /* NVIC配置 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    /* 使能TIM2 */
    TIM_Cmd(TIM2, ENABLE);
}




