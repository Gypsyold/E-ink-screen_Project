#ifndef SPI_BSP_H
#define SPI_BSP_H

/*
 * spi_bsp.h — SPI + DMA abstraction layer.
 *
 * Design principles:
 *   1. Upper layers (W25Q128, EPD drivers) never call HAL_SPI_* directly.
 *   2. CS (chip-select) is NOT managed here — the driver controls CS
 *      so it can keep CS low across a multi-phase transaction
 *      (e.g., command byte + DMA data payload).
 *   3. DMA functions present a synchronous interface: they start DMA,
 *      block the calling FreeRTOS task on a semaphore, and return only
 *      after the transfer is complete.  The task yields during the wait.
 *   4. HAL_SPI_TxCpltCallback is defined here and dispatches to the
 *      correct semaphore — no other file should define it.
 *
 * Initialization:
 *   Call SPI_BSP_Init() once in main.c (USER CODE BEGIN 2, before
 *   osKernelStart) to create the internal DMA semaphores.
 *
 * DMA functions must be called from a FreeRTOS task (after osKernelStart).
 * Blocking functions can be called from anywhere.
 */

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* Create DMA completion semaphores — call before osKernelStart */
void SPI_BSP_Init(void);

/* ---- Blocking transfers (suitable for short commands / single bytes) ---- */
void SPI_BSP_Transmit(SPI_HandleTypeDef *hspi, const uint8_t *buf, uint16_t len);
void SPI_BSP_Receive(SPI_HandleTypeDef *hspi, uint8_t *buf, uint16_t len);
void SPI_BSP_TransmitReceive(SPI_HandleTypeDef *hspi,
                              const uint8_t *tx, uint8_t *rx, uint16_t len);

/* ---- DMA transfers (task-blocking, efficient for large payloads) ---- */

/* Send buf[len] via DMA; task blocks until transfer complete */
void SPI_BSP_TransmitDMA(SPI_HandleTypeDef *hspi, const uint8_t *buf, uint16_t len);

/* Fill count bytes with constant val via chunked DMA (no large RAM needed) */
void SPI_BSP_FillDMA(SPI_HandleTypeDef *hspi, uint8_t val, uint16_t count);

#endif /* SPI_BSP_H */
