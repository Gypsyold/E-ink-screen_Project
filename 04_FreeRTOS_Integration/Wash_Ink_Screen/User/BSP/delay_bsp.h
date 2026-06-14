#ifndef DELAY_BSP_H
#define DELAY_BSP_H

/*
 * delay_bsp.h — delay and tick abstraction.
 *
 * When the FreeRTOS scheduler is running, Delay_BSP_ms uses vTaskDelay
 * so the calling task yields during the wait (CPU-friendly).
 * Before the scheduler starts (e.g., in main before osKernelStart),
 * it falls back to HAL_Delay.
 *
 * Upper layers never call HAL_Delay or vTaskDelay directly, so switching
 * RTOS or bare-metal later only requires changes here.
 */

#include <stdint.h>

void     Delay_BSP_ms(uint32_t ms);
uint32_t Delay_BSP_Tick(void);

#endif /* DELAY_BSP_H */
