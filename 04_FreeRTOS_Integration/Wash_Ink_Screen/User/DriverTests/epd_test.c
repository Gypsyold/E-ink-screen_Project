#include "epd_test.h"
#include "epd.h"
#include "epd_gui.h"
#include "board_bsp.h"
#include "delay_bsp.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/* 帧缓冲（静态分配，避免栈溢出） */
static uint8_t s_frame[EPD_FRAME_BYTES];

/* ================================================================
 * UART 调试输出
 * ================================================================ */
static void uart_print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
}

static void uart_logfmt(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_print(buf);
}

/* ================================================================
 * EPD_Test — EPD 显示测试（在 FreeRTOS 任务中调用）
 * ================================================================ */
void EPD_Test(void)
{
    uart_print("\r\n");
    uart_print("================================\r\n");
    uart_print("  EPD 中文显示测试\r\n");
    uart_print("================================\r\n\r\n");

    /* ---- 初始化画布 ---- */
    uart_print("[1] 初始化画布...\r\n");
    Canvas_Init(s_frame, 0, GUI_WHITE);
    Canvas_Clear(GUI_WHITE);
    uart_print("    完成\r\n\r\n");

    /* ---- 初始化 EPD ---- */
    uart_print("[2] 初始化 EPD（等待 BUSY 信号）...\r\n");
    EPD_Init();
    uart_print("    完成\r\n\r\n");

    /* ---- 清屏 ---- */
    uart_print("[3] 清屏为白色...\r\n");
    EPD_Display_Clear();
    EPD_Update();
    Delay_BSP_ms(2000);
    uart_print("    完成\r\n\r\n");

    /* ---- ASCII 显示 ---- */
    uart_print("[4] 绘制 ASCII 字符串...\r\n");
    Canvas_Clear(GUI_WHITE);
    GUI_ShowString( 0,  0, "EPD Driver Test",  FONT_16X16, GUI_BLACK);
    GUI_ShowString( 0, 20, "Font: 8x16 ASCII", FONT_16X16, GUI_BLACK);
    GUI_ShowString( 0, 40, "0123456789",       FONT_16X16, GUI_BLACK);
    GUI_ShowString( 0, 60, "12x24: ABCDE",     FONT_24X24, GUI_BLACK);
    EPD_Display(s_frame);
    EPD_Update();
    Delay_BSP_ms(3000);
    uart_print("    完成\r\n\r\n");

    /* ---- 16×16 中文显示 ---- */
    uart_print("[5] 绘制 16x16 中文（从 Flash 读字模）...\r\n");
    Canvas_Clear(GUI_WHITE);

    /* "水墨屏测试" GBK 编码 */
    static const uint8_t msg16[] = {
        0xCB, 0xAE,   /* 水 */
        0xC4, 0xEA,   /* 墨 */
        0xC6, 0xC1,   /* 屏 */
        0xB2, 0xE2,   /* 测 */
        0xCA, 0xD4,   /* 试 */
        0x00
    };
    uart_logfmt("    Flash 基址: 0x%06lX\r\n", (unsigned long)W25Q128_FONT16_ADDR);
    GUI_ShowChineseStr(0, 0, msg16, FONT_16X16, GUI_BLACK);
    GUI_ShowString(0, 20, "16x16 GBK OK", FONT_16X16, GUI_BLACK);
    EPD_Display(s_frame);
    EPD_Update();
    Delay_BSP_ms(3000);
    uart_print("    完成\r\n\r\n");

    /* ---- 24×24 中文显示 ---- */
    uart_print("[6] 绘制 24x24 中文（从 Flash 读字模）...\r\n");
    Canvas_Clear(GUI_WHITE);

    /* "显示" GBK 编码 */
    static const uint8_t msg24[] = {
        0xCF, 0xD4,   /* 显 */
        0xCA, 0xBE,   /* 示 */
        0x00
    };
    uart_logfmt("    Flash 基址: 0x%06lX\r\n", (unsigned long)W25Q128_FONT24_ADDR);
    GUI_ShowChineseStr(0, 0, msg24, FONT_24X24, GUI_BLACK);
    GUI_ShowString(0, 28, "24x24 GBK OK", FONT_16X16, GUI_BLACK);
    EPD_Display(s_frame);
    EPD_Update();
    Delay_BSP_ms(3000);
    uart_print("    完成\r\n\r\n");

    /* ---- 混合显示（ASCII + 汉字同一函数） ---- */
    uart_print("[7] 混合显示（GUI_ShowText）...\r\n");
    Canvas_Clear(GUI_WHITE);

    /* 第一行 16×16："水墨屏-EPD" */
    static const uint8_t mix16[] = {
        0xCB, 0xAE,          /* 水 */
        0xC4, 0xEA,          /* 墨 */
        0xC6, 0xC1,          /* 屏 */
        '-', 'E', 'P', 'D',
        0x00
    };
    /* 第二行 24×24："显示 24x24" */
    static const uint8_t mix24[] = {
        0xCF, 0xD4,          /* 显 */
        0xCA, 0xBE,          /* 示 */
        ' ', '2', '4', 'x', '2', '4',
        0x00
    };

    GUI_ShowText(  0,  0, "水墨屏-EPD", FONT_16X16, GUI_BLACK);
    GUI_ShowText(  0, 30, mix24, FONT_24X24, GUI_BLACK);

    EPD_Display(s_frame);
    EPD_Update();
    Delay_BSP_ms(3000);
    uart_print("    完成\r\n\r\n");

    /* ---- 深度睡眠 ---- */
    uart_print("[8] 进入深度睡眠\r\n");
    EPD_DeepSleep();

    uart_print("\r\n================================\r\n");
    uart_print("  测试结束\r\n");
    uart_print("================================\r\n");

    for (;;) {
        Delay_BSP_ms(1000);
    }
}
