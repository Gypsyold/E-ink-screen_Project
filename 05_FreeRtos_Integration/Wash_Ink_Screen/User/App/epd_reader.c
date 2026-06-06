#include "epd_reader.h"
#include "epd.h"
#include "epd_gui.h"
#include "sd_card.h"
#include "fatfs.h"
#include "board_bsp.h"
#include "delay_bsp.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define READER_BUF_SIZE   512u
#define LINE_H            16u    /* FONT_16X16 line height */
#define DISPLAY_W         296u   /* GUI x pixels */
#define DISPLAY_H         152u   /* GUI y pixels */

static uint8_t s_frame[EPD_FRAME_BYTES];
static uint8_t s_file_buf[READER_BUF_SIZE];

/* ------------------------------------------------------------------ */
static void uart_print(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), HAL_MAX_DELAY);
}

static void uart_fmt(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    uart_print(buf);
}
/* ------------------------------------------------------------------ */

/* ================================================================
 * EPD_SD_ShowFile
 * Read GBK text file from SD card, display line by line on EPD.
 * Auto word-wrap at display width (296px).
 * Note: file content is GBK bytes. Chinese will appear garbled in
 *       UTF-8 serial terminal but displays correctly on EPD.
 * ================================================================ */
void EPD_SD_ShowFile(const char *sd_path)
{
    uart_print("\r\n================================\r\n");
    uart_fmt("  EPD_SD_ShowFile: %s\r\n", sd_path);
    uart_print("================================\r\n\r\n");

    uart_print("[1] Mount SD...\r\n");
    SD_Err err = SD_Mount();
    if (err != SD_OK) {
        uart_fmt("    FAIL code=%d\r\n", (int)err);
        return;
    }
    uart_print("    OK\r\n");

    uart_fmt("[2] Read: %s\r\n", sd_path);
    uint32_t read_len = 0u;
    err = SD_ReadFile(sd_path, s_file_buf, READER_BUF_SIZE, &read_len);
    SD_Unmount();
    if (err != SD_OK) {
        uart_fmt("    FAIL code=%d\r\n", (int)err);
        return;
    }
    uart_fmt("    %lu bytes\r\n\r\n", (unsigned long)read_len);

    uart_print("[3] EPD init...\r\n");
    Canvas_Init(s_frame, 0u, GUI_WHITE);
    Canvas_Clear(GUI_WHITE);
    EPD_Init();
    EPD_Display_Clear();
    EPD_Update();
    Delay_BSP_ms(2000);
    uart_print("    OK\r\n\r\n");

    uart_print("[4] Render lines:\r\n");
    uart_print("----\r\n");

    Canvas_Clear(GUI_WHITE);

    uint16_t y       = 0u;
    uint8_t  line_no = 0u;
    uint8_t *p       = s_file_buf;
    uint8_t *end_ptr = s_file_buf + read_len;

    /* segment buffer: max 18 GBK chars = 36 bytes, use 64 for margin */
    uint8_t  seg[64];
    uint8_t  seg_len = 0u;
    uint16_t x       = 0u;

    #define FLUSH_SEG() do {                                               \
        if (seg_len > 0u && (y + LINE_H) <= DISPLAY_H) {                  \
            seg[seg_len] = '\0';                                           \
            GUI_ShowText(0u, y, seg, FONT_16X16, GUI_BLACK);              \
            uart_fmt("L%d: %s\r\n", (int)line_no, (char *)seg);           \
            y += LINE_H;                                                   \
            line_no++;                                                     \
        }                                                                  \
        seg_len = 0u; x = 0u;                                             \
    } while (0)

    while (p < end_ptr) {
        if (*p == '\r' || *p == '\n') {
            FLUSH_SEG();
            if (*p == '\r') p++;
            if (p < end_ptr && *p == '\n') p++;
            continue;
        }

        uint8_t  ch_bytes;
        uint16_t ch_width;
        if (*p >= 0x81u && (p + 1u) < end_ptr && *(p + 1u) >= 0x40u) {
            ch_bytes = 2u;
            ch_width = 16u;
        } else {
            ch_bytes = 1u;
            ch_width = 8u;
        }

        if (x + ch_width > DISPLAY_W) {
            FLUSH_SEG();
        }

        if ((y + LINE_H) > DISPLAY_H) break;

        if (seg_len + ch_bytes < (uint8_t)sizeof(seg)) {
            for (uint8_t i = 0u; i < ch_bytes; i++) {
                seg[seg_len++] = *p++;
            }
            x += ch_width;
        } else {
            p += ch_bytes;
        }
    }

    FLUSH_SEG();
    #undef FLUSH_SEG

    uart_print("----\r\n");
    uart_fmt("Total: %d lines\r\n\r\n", (int)line_no);

    uart_print("[5] EPD refresh...\r\n");
    EPD_Display(s_frame);
    EPD_Update();
    uart_print("    OK\r\n");
    uart_print("[6] EPD deep sleep\r\n");
    EPD_DeepSleep();

    uart_print("\r\n================================\r\n");
    uart_print("  Done\r\n");
    uart_print("================================\r\n");
}

/* ================================================================
 * EPD_SD_ListFiles
 * Scan directory, display numbered file list on EPD and UART.
 * Note: Chinese filenames from FatFS are GBK bytes. They display
 *       correctly on EPD but will appear garbled in UTF-8 terminal.
 * ================================================================ */
void EPD_SD_ListFiles(const char *dir_path)
{
    uart_print("\r\n================================\r\n");
    uart_fmt("  EPD_SD_ListFiles: %s\r\n", dir_path);
    uart_print("================================\r\n\r\n");

    uart_print("[1] Mount SD...\r\n");
    SD_Err err = SD_Mount();
    if (err != SD_OK) {
        uart_fmt("    FAIL code=%d\r\n", (int)err);
        return;
    }
    uart_print("    OK\r\n\r\n");

    uart_print("[2] EPD init...\r\n");
    Canvas_Init(s_frame, 0u, GUI_WHITE);
    Canvas_Clear(GUI_WHITE);
    EPD_Init();
    EPD_Display_Clear();
    EPD_Update();
    Delay_BSP_ms(2000);
    uart_print("    OK\r\n\r\n");

    uart_print("[3] File list:\r\n");
    uart_print("----\r\n");

    Canvas_Clear(GUI_WHITE);

    DIR      dir;
    FILINFO  fno;
    char     line[64];
    uint8_t  file_idx = 0u;
    uint16_t y        = 0u;

    if (f_opendir(&dir, dir_path) == FR_OK) {
        while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0') {
            if (fno.fattrib & AM_DIR) continue;

            file_idx++;
            snprintf(line, sizeof(line), "[%d] %s", (int)file_idx, fno.fname);
            uart_fmt("%s\r\n", line);

            if ((y + LINE_H) <= DISPLAY_H) {
                GUI_ShowText(0u, y, (uint8_t *)line, FONT_16X16, GUI_BLACK);
                y += LINE_H;
            }
        }
        f_closedir(&dir);
    } else {
        uart_print("    open dir FAIL\r\n");
    }

    SD_Unmount();
    uart_print("----\r\n");
    uart_fmt("Total: %d files\r\n\r\n", (int)file_idx);

    uart_print("[4] EPD refresh...\r\n");
    EPD_Display(s_frame);
    EPD_Update();
    uart_print("    OK\r\n");
    uart_print("[5] EPD deep sleep\r\n");
    EPD_DeepSleep();

    uart_print("\r\n================================\r\n");
    uart_print("  Done\r\n");
    uart_print("================================\r\n");
}
