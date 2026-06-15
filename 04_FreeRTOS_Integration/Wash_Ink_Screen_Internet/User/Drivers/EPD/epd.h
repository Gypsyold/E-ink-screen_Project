#ifndef EPD_H
#define EPD_H

/*
 * epd.h — SSD1681-compatible 2.66" e-ink display driver (152x296, B&W).
 *
 * Dependencies (BSP layer only — no direct HAL calls):
 *   board_bsp.h  gpio_bsp.h  spi_bsp.h  delay_bsp.h
 *
 * All functions must be called from a FreeRTOS task context:
 *   - EPD_WaitBusy uses Delay_BSP_ms which yields via vTaskDelay.
 *   - EPD_Display / EPD_Display_Clear use SPI_BSP_FillDMA / TransmitDMA
 *     which block on a DMA semaphore.
 *
 * Typical usage sequence:
 *   EPD_Init();              // full init (normal refresh)
 *   EPD_Display_Clear();     // clear both RAM banks to white
 *   EPD_Update();            // trigger display refresh
 *   EPD_Display(my_image);   // push new frame
 *   EPD_Update();            // refresh
 *   EPD_DeepSleep();         // low-power sleep when done
 */

#include <stdint.h>

#define EPD_WIDTH    152
#define EPD_HEIGHT   296
#define EPD_WHITE    0xFF
#define EPD_BLACK    0x00

/* Frame buffer size: (152/8) * 296 = 5624 bytes */
#define EPD_FRAME_BYTES  ((EPD_WIDTH / 8) * EPD_HEIGHT)

/* ---- Low-level primitives ---- */
void EPD_HW_Reset(void);
void EPD_WaitBusy(void);
void EPD_WriteCmd(uint8_t cmd);
void EPD_WriteData(uint8_t dat);

/* ---- Update triggers ---- */
void EPD_Update(void);        /* full refresh (normal mode) */
void EPD_FastUpdate(void);    /* fast refresh               */
void EPD_PartUpdate(void);    /* partial refresh            */

/* ---- Initialization ---- */
void EPD_Init(void);          /* full init, normal refresh mode */
void EPD_FastInit(void);      /* fast refresh mode              */

/* ---- Display operations (DMA, call from task) ---- */
void EPD_Display(const uint8_t *image);       /* push 5624-byte frame to RAM 0x24          */
void EPD_DisplayPrev(const uint8_t *image);   /* push 5624-byte frame to RAM 0x26 (prev)   */
void EPD_Display_Clear(void);                 /* both RAM banks → white                    */
void EPD_Clear_R26H(void);                    /* clear RAM 0x26 only                       */
void EPD_PartInit(void);                      /* set border for partial mode (no HW reset) */

/* ---- Power ---- */
void EPD_DeepSleep(void);

#endif /* EPD_H */
