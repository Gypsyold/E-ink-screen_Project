#include "spi_bsp.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <string.h>

/*
 * One binary semaphore per SPI bus.
 * The semaphore starts empty; HAL_SPI_TxCpltCallback gives it when DMA done.
 * The task calling SPI_BSP_TransmitDMA / SPI_BSP_FillDMA takes it to wait.
 *
 * s_fill_buf: shared 256-byte scratch buffer for SPI_BSP_FillDMA.
 * Must reside in RAM (not Flash) for DMA access.
 * Do not call SPI_BSP_FillDMA from two tasks simultaneously.
 */

static SemaphoreHandle_t s_spi1_sem = NULL;
static SemaphoreHandle_t s_spi2_sem = NULL;

static uint8_t s_fill_buf[256];   /* RAM, DMA-safe */

/* ---- init ---- */

void SPI_BSP_Init(void)
{
    s_spi1_sem = xSemaphoreCreateBinary();
    s_spi2_sem = xSemaphoreCreateBinary();
}

/* ---- HAL DMA TX complete callback — centralized here ---- */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    BaseType_t woken = pdFALSE;
    if (hspi->Instance == SPI1 && s_spi1_sem)
        xSemaphoreGiveFromISR(s_spi1_sem, &woken);
    else if (hspi->Instance == SPI2 && s_spi2_sem)
        xSemaphoreGiveFromISR(s_spi2_sem, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ---- internal helper: get the semaphore for a given SPI handle ---- */

static SemaphoreHandle_t spi_sem(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1) return s_spi1_sem;
    if (hspi->Instance == SPI2) return s_spi2_sem;
    return NULL;
}

/* ---- blocking transfers ---- */

void SPI_BSP_Transmit(SPI_HandleTypeDef *hspi, const uint8_t *buf, uint16_t len)
{
    HAL_SPI_Transmit(hspi, (uint8_t *)buf, len, HAL_MAX_DELAY);
}

void SPI_BSP_Receive(SPI_HandleTypeDef *hspi, uint8_t *buf, uint16_t len)
{
    HAL_SPI_Receive(hspi, buf, len, HAL_MAX_DELAY);
}

void SPI_BSP_TransmitReceive(SPI_HandleTypeDef *hspi,
                              const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    HAL_SPI_TransmitReceive(hspi, (uint8_t *)tx, rx, len, HAL_MAX_DELAY);
}

/* ---- DMA transfers ---- */

void SPI_BSP_TransmitDMA(SPI_HandleTypeDef *hspi, const uint8_t *buf, uint16_t len)
{
    SemaphoreHandle_t sem = spi_sem(hspi);
    if (!sem) return;

    xSemaphoreTake(sem, 0);                           /* drain any stale token */
    HAL_SPI_Transmit_DMA(hspi, (uint8_t *)buf, len);
    xSemaphoreTake(sem, portMAX_DELAY);               /* wait for ISR */
}

void SPI_BSP_FillDMA(SPI_HandleTypeDef *hspi, uint8_t val, uint16_t count)
{
    SemaphoreHandle_t sem = spi_sem(hspi);
    if (!sem) return;

    memset(s_fill_buf, val, sizeof(s_fill_buf));

    for (uint16_t rem = count; rem > 0; ) {
        uint16_t chunk = (rem < (uint16_t)sizeof(s_fill_buf))
                         ? rem : (uint16_t)sizeof(s_fill_buf);
        xSemaphoreTake(sem, 0);
        HAL_SPI_Transmit_DMA(hspi, s_fill_buf, chunk);
        xSemaphoreTake(sem, portMAX_DELAY);
        rem -= chunk;
    }
}
