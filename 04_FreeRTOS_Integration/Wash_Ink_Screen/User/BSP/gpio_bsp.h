#ifndef GPIO_BSP_H
#define GPIO_BSP_H

/*
 * gpio_bsp.h — GPIO abstraction layer.
 *
 * Upper layers (drivers) call these instead of HAL_GPIO_WritePin /
 * HAL_GPIO_ReadPin directly, so hardware details stay in the BSP.
 */

#include "stm32f4xx_hal.h"
#include <stdint.h>

void    GPIO_BSP_Set(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void    GPIO_BSP_Clr(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
uint8_t GPIO_BSP_Read(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

#endif /* GPIO_BSP_H */
