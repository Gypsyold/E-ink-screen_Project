#include "gpio_bsp.h"

void GPIO_BSP_Set(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_SET);
}

void GPIO_BSP_Clr(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    HAL_GPIO_WritePin(GPIOx, GPIO_Pin, GPIO_PIN_RESET);
}

uint8_t GPIO_BSP_Read(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    return (uint8_t)HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
}
