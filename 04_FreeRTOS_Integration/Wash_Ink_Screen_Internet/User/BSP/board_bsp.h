#ifndef BOARD_BSP_H
#define BOARD_BSP_H

/*
 * board_bsp.h — Hardware mapping for this board.
 *
 * This file is the single source of truth for "who is connected where".
 * It does NOT contain any logic — only GPIO definitions and SPI handle externs.
 * Upper layers (Drivers) include this to know the hardware topology.
 */

#include "stm32f4xx_hal.h"

/* ================================================================
 * UART1 — 调试串口，115200 baud，printf 输出
 * ================================================================ */
extern UART_HandleTypeDef huart1;    /* main.c 由 CubeMX 定义 */
extern UART_HandleTypeDef huart2;    /* USART2: PA2=TX  PA3=RX，WiFi模块通信 */

/* ================================================================
 * Keys — GPIO input, internal pull-up, active low
 * PC0 / PC1 / PC2 / PC3
 * ================================================================ */
#define KEY1_PORT   GPIOC
#define KEY1_PIN    GPIO_PIN_0
#define KEY2_PORT   GPIOC
#define KEY2_PIN    GPIO_PIN_1
#define KEY3_PORT   GPIOC
#define KEY3_PIN    GPIO_PIN_2
#define KEY4_PORT   GPIOC
#define KEY4_PIN    GPIO_PIN_3
#define KEY_ACTIVE  GPIO_PIN_RESET   /* pressed = low (pull-up) */

/* ================================================================
 * W25Q128 Flash — SPI1, CS = PA4
 * ================================================================ */
#define W25Q_CS_PORT    GPIOA
#define W25Q_CS_PIN     GPIO_PIN_4
extern SPI_HandleTypeDef hspi1;      /* defined in main.c by CubeMX */

/* ================================================================
 * EPD e-ink display 152x296 — SPI2
 *   CS   = PB12
 *   DC   = PC5   (0=command, 1=data)
 *   RST  = PC4   (active low)
 *   BUSY = PC6   (1=busy, 0=ready)
 * ================================================================ */
#define EPD_CS_PORT     GPIOB
#define EPD_CS_PIN      GPIO_PIN_12
#define EPD_DC_PORT     GPIOC
#define EPD_DC_PIN      GPIO_PIN_5
#define EPD_RST_PORT    GPIOC
#define EPD_RST_PIN     GPIO_PIN_4
#define EPD_BUSY_PORT   GPIOC
#define EPD_BUSY_PIN    GPIO_PIN_6
#define EPD_BUSY_ACTIVE GPIO_PIN_SET /* HIGH = busy */
extern SPI_HandleTypeDef hspi2;      /* defined in main.c by CubeMX */

/* ================================================================
 * SD Card — SDIO 4-bit，卡检测 = PD3（上拉输入，低电平=已插入）
 * ================================================================ */
#define SD_DET_PORT     GPIOD
#define SD_DET_PIN      GPIO_PIN_3
extern SD_HandleTypeDef hsd;         /* defined in main.c by CubeMX */

#endif /* BOARD_BSP_H */
