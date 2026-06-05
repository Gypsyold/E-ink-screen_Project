#include "epd_test.h"
#include "epd.h"
#include "delay_bsp.h"

/*
 * EPD test suite.
 *
 * Test 1 — Normal init + clear + full refresh
 *   Verifies: HW_Reset, SPI cmd/data path, DMA fill, Update timing.
 *   Expected: screen turns fully white.
 *
 * Test 2 — Display solid black
 *   Verifies: DMA frame transfer (5624 bytes), EPD_Display API.
 *   Expected: screen turns fully black.
 *
 * Test 3 — Fast init + white clear
 *   Verifies: EPD_FastInit / EPD_FastUpdate sequence.
 *   Expected: screen turns white quickly (no full waveform cycle).
 *
 * Test 4 — Deep sleep
 *   Verifies: EPD_DeepSleep command accepted (no hardfault).
 *   After sleep the display must be power-cycled or HW_Reset before reuse.
 */

/* All-black frame (5624 bytes of 0x00); in flash, DMA-accessible on STM32F407 */
static const uint8_t s_black_frame[EPD_FRAME_BYTES];   /* zero-init = black */

/* All-white frame (all 0xFF) — generated on first use to save flash */
static uint8_t s_white_frame[EPD_FRAME_BYTES];
static uint8_t s_white_frame_ready = 0;

static void prepare_white_frame(void)
{
    if (!s_white_frame_ready) {
        for (uint16_t i = 0; i < EPD_FRAME_BYTES; i++)
            s_white_frame[i] = EPD_WHITE;
        s_white_frame_ready = 1;
    }
}

/* ------------------------------------------------------------------ */

void EPD_Test(void)
{
    /* --- Test 1: full init, clear to white, refresh --- */
    EPD_Init();
    EPD_Display_Clear();
    EPD_Update();
    Delay_BSP_ms(2000);   /* observe: all white */

    /* --- Test 2: display solid black --- */
    EPD_Display(s_black_frame);
    EPD_Update();
    Delay_BSP_ms(2000);   /* observe: all black */

    /* --- Test 3: fast init, clear to white --- */
    prepare_white_frame();
    EPD_FastInit();
    EPD_Display(s_white_frame);
    EPD_FastUpdate();
    Delay_BSP_ms(1000);   /* observe: all white (fast waveform) */

    /* --- Test 4: enter deep sleep --- */
    EPD_DeepSleep();
    /* EPD is now in deep sleep; further operations require EPD_Init() again */
}
