#include "epd.h"
#include "board_bsp.h"
#include "gpio_bsp.h"
#include "spi_bsp.h"
#include "delay_bsp.h"

/*
 * EPD driver for SSD1681-compatible 2.66" e-ink (152x296).
 *
 * This file contains ONLY EPD controller protocol logic.
 * No direct HAL calls. No CS/DC toggling scattered across helper functions.
 *
 * CS and DC are managed at the command/data level:
 *   - EPD_WriteCmd:  DC=0, CS low, send 1 byte, CS high, DC=1
 *   - EPD_WriteData: DC=1, CS low, send 1 byte, CS high
 *   - DMA bulk send: DC=1, CS low for entire transfer, CS high
 *
 * The driver calls BSP functions:
 *   GPIO_BSP_Set/Clr  →  CS, DC, RST control
 *   GPIO_BSP_Read     →  BUSY polling
 *   SPI_BSP_Transmit  →  single-byte blocking send
 *   SPI_BSP_TransmitDMA / FillDMA  →  large payloads via DMA
 *   Delay_BSP_ms      →  yields to FreeRTOS during waits
 */

/* ---- CS / DC inline helpers ---- */

static inline void cs_low(void)  { GPIO_BSP_Clr(EPD_CS_PORT,  EPD_CS_PIN);  }
static inline void cs_high(void) { GPIO_BSP_Set(EPD_CS_PORT,  EPD_CS_PIN);  }
static inline void dc_cmd(void)  { GPIO_BSP_Clr(EPD_DC_PORT,  EPD_DC_PIN);  }
static inline void dc_data(void) { GPIO_BSP_Set(EPD_DC_PORT,  EPD_DC_PIN);  }

/* ================================================================
 * Low-level primitives
 * ================================================================ */

void EPD_WriteCmd(uint8_t cmd)
{
    dc_cmd();
    cs_low();
    SPI_BSP_Transmit(&hspi2, &cmd, 1);
    cs_high();
    dc_data();   /* return DC to data state as default */
}

void EPD_WriteData(uint8_t dat)
{
    dc_data();
    cs_low();
    SPI_BSP_Transmit(&hspi2, &dat, 1);
    cs_high();
}

void EPD_WaitBusy(void)
{
    /* BUSY pin HIGH = controller is processing; wait until it goes LOW */
    while (GPIO_BSP_Read(EPD_BUSY_PORT, EPD_BUSY_PIN) == EPD_BUSY_ACTIVE)
        Delay_BSP_ms(1);
}

void EPD_HW_Reset(void)
{
    Delay_BSP_ms(100);
    GPIO_BSP_Clr(EPD_RST_PORT, EPD_RST_PIN);
    Delay_BSP_ms(10);
    GPIO_BSP_Set(EPD_RST_PORT, EPD_RST_PIN);
    Delay_BSP_ms(10);
    EPD_WaitBusy();
}

/* ================================================================
 * Internal DMA helpers — CS held low for the entire transfer
 * ================================================================ */

static void epd_send_buffer_dma(const uint8_t *buf, uint16_t len)
{
    dc_data();
    cs_low();
    SPI_BSP_TransmitDMA(&hspi2, buf, len);
    cs_high();
}

static void epd_fill_dma(uint8_t val, uint16_t count)
{
    dc_data();
    cs_low();
    SPI_BSP_FillDMA(&hspi2, val, count);
    cs_high();
}

/* ================================================================
 * Update triggers
 * ================================================================ */

void EPD_Update(void)
{
    EPD_WriteCmd(0x22);
    EPD_WriteData(0xF4);
    EPD_WriteCmd(0x20);
    EPD_WaitBusy();
}

void EPD_FastUpdate(void)
{
    EPD_WriteCmd(0x22);
    EPD_WriteData(0xC7);
    EPD_WriteCmd(0x20);
    EPD_WaitBusy();
}

void EPD_PartUpdate(void)
{
    EPD_WriteCmd(0x22);
    EPD_WriteData(0x1C);
    EPD_WriteCmd(0x20);
    EPD_WaitBusy();
}

/* ================================================================
 * Initialization
 * ================================================================ */

void EPD_Init(void)
{
    EPD_HW_Reset();
    EPD_WaitBusy();

    EPD_WriteCmd(0x12);   /* SWRESET */
    EPD_WaitBusy();

    EPD_WriteCmd(0x3C);   /* BorderWaveform */
    EPD_WriteData(0x05);

    EPD_WriteCmd(0x01);   /* Driver output control */
    EPD_WriteData((EPD_HEIGHT - 1) % 256);
    EPD_WriteData((EPD_HEIGHT - 1) / 256);
    EPD_WriteData(0x00);

    EPD_WriteCmd(0x11);   /* Data entry mode: X dec, Y inc */
    EPD_WriteData(0x02);

    EPD_WriteCmd(0x44);   /* RAM-X address start/end */
    EPD_WriteData(EPD_WIDTH / 8 - 1);
    EPD_WriteData(0x00);

    EPD_WriteCmd(0x45);   /* RAM-Y address start/end */
    EPD_WriteData(0x00);
    EPD_WriteData(0x00);
    EPD_WriteData((EPD_HEIGHT - 1) % 256);
    EPD_WriteData((EPD_HEIGHT - 1) / 256);

    EPD_WriteCmd(0x21);   /* Display update control */
    EPD_WriteData(0x00);
    EPD_WriteData(0x80);

    EPD_WriteCmd(0x18);   /* Built-in temperature sensor */
    EPD_WriteData(0x80);

    EPD_WriteCmd(0x4E);   /* RAM X address counter */
    EPD_WriteData(EPD_WIDTH / 8 - 1);

    EPD_WriteCmd(0x4F);   /* RAM Y address counter */
    EPD_WriteData(0x00);
    EPD_WriteData(0x00);

    EPD_WaitBusy();
}

void EPD_FastInit(void)
{
    EPD_HW_Reset();
    EPD_WaitBusy();

    EPD_WriteCmd(0x18);
    EPD_WriteData(0x80);
    EPD_WriteCmd(0x22);
    EPD_WriteData(0xB1);
    EPD_WriteCmd(0x20);
    EPD_WaitBusy();

    EPD_WriteCmd(0x1A);
    EPD_WriteData(0x64);
    EPD_WriteData(0x00);
    EPD_WriteCmd(0x22);
    EPD_WriteData(0x91);
    EPD_WriteCmd(0x20);
    EPD_WaitBusy();
}

/* ================================================================
 * Display operations
 * ================================================================ */

void EPD_Display(const uint8_t *image)
{
    EPD_WriteCmd(0x24);
    epd_send_buffer_dma(image, EPD_FRAME_BYTES);
}

void EPD_Display_Clear(void)
{
    EPD_WriteCmd(0x3C);
    EPD_WriteData(0x01);

    EPD_WriteCmd(0x24);
    epd_fill_dma(EPD_WHITE, EPD_FRAME_BYTES);

    EPD_WriteCmd(0x26);
    epd_fill_dma(EPD_WHITE, EPD_FRAME_BYTES);
}

void EPD_Clear_R26H(void)
{
    EPD_WaitBusy();
    EPD_WriteCmd(0x26);
    epd_fill_dma(EPD_WHITE, EPD_FRAME_BYTES);
}

/* ================================================================
 * Power
 * ================================================================ */

void EPD_DeepSleep(void)
{
    EPD_WriteCmd(0x10);
    EPD_WriteData(0x01);
    Delay_BSP_ms(200);
}
