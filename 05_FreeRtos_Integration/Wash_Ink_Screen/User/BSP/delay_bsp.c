#include "delay_bsp.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

void Delay_BSP_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        vTaskDelay(pdMS_TO_TICKS(ms));
    else
        HAL_Delay(ms);
}

uint32_t Delay_BSP_Tick(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        return (uint32_t)xTaskGetTickCount();
    return HAL_GetTick();
}
