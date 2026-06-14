#ifndef EPD_GUI_H
#define EPD_GUI_H

/*
 * epd_gui.h — EPD 图形界面层
 *
 * 坐标系（Rotate=0，横屏模式）：
 *   x = 水平方向 (0..295，向右增大)
 *   y = 垂直方向 (0..151，向下增大)
 *
 * 颜色：
 *   GUI_WHITE = 0xFF（白色，屏幕不点亮）
 *   GUI_BLACK = 0x00（黑色，屏幕点亮）
 *
 * Flash 字库地址（只读，禁止写入）：
 *   16×16 GBK 字库：0x000000
 *   24×24 GBK 字库：0x0C0000
 *
 * ASCII 字体（内嵌 ROM，无需 Flash）：
 *   FONT_16X16 → 8×16 点阵（asc2_1608）
 *   FONT_24X24 → 12×24 点阵（asc2_2412）
 *
 * GBK 中文字体：
 *   从 W25Q128 Flash 读取，需在 FreeRTOS 任务中调用
 */

#include <stdint.h>

/* ---- 颜色定义 ---- */
#define GUI_WHITE   0xFFu
#define GUI_BLACK   0x00u

/* ---- Flash 字库基地址（禁止对这两个地址以下区域执行写操作）---- */
#define W25Q128_FONT16_ADDR   0x000000u   /* 16×16 GBK 楷体，每字 32 字节 */
#define W25Q128_FONT24_ADDR   0x0C0000u   /* 24×24 GBK 楷体，每字 72 字节 */
#define W25Q128_USER_ADDR     0x280000u   /* 用户数据区起始 */

/* ---- 字体大小枚举 ---- */
typedef enum {
    FONT_16X16 = 0,
    FONT_24X24 = 1,
} FontSize;

/* ---- 画布结构体 ---- */
typedef struct {
    uint8_t  *image;       /* 帧缓冲指针（5624 字节）           */
    uint16_t  width_mem;   /* 物理列数（152）                    */
    uint16_t  height_mem;  /* 物理行数（296）                    */
    uint16_t  width_byte;  /* 每行字节数（19）                   */
    uint16_t  rotate;      /* 旋转角度（0/90/180/270）           */
    uint16_t  bg_color;    /* 背景色                             */
} EPD_Canvas;

extern EPD_Canvas g_canvas;

/* ================================================================
 * 画布基础操作
 * ================================================================ */

/* 初始化画布（buf 必须为 EPD_FRAME_BYTES 大小的静态缓冲区） */
void Canvas_Init(uint8_t *buf, uint16_t rotate, uint16_t bg_color);

/* 清空画布为指定颜色 */
void Canvas_Clear(uint8_t color);

/* 设置单个像素（x=水平 0..295，y=垂直 0..151） */
void Canvas_SetPixel(uint16_t x, uint16_t y, uint8_t color);

/* ================================================================
 * ASCII 显示（使用内嵌 8×16 点阵字体）
 * ================================================================ */

/* 显示单个 ASCII 字符
 *   FONT_16X16 → 8×16 点阵字体
 *   FONT_24X24 → 12×24 点阵字体 */
void GUI_ShowChar(uint16_t x, uint16_t y, char ch, FontSize fs, uint8_t color);

/* 显示 ASCII 字符串（字符宽随 fs 自动选择：8 或 12px） */
void GUI_ShowString(uint16_t x, uint16_t y, const char *str, FontSize fs, uint8_t color);

/* 显示无符号整数（digits=显示位数） */
void GUI_ShowNum(uint16_t x, uint16_t y, uint32_t num, uint8_t digits, FontSize fs, uint8_t color);

/* ================================================================
 * 中文显示（从 W25Q128 Flash 读取 GBK 字模）
 * 必须在 FreeRTOS 任务中调用（W25Q128_Read 会阻塞等待 SPI）
 * ================================================================ */

/* 显示单个中文字符（gbk2 为 2 字节 GBK 编码） */
void GUI_ShowChinese(uint16_t x, uint16_t y, const uint8_t *gbk2, FontSize fs, uint8_t color);

/* 显示中文字符串（str 为 GBK 编码，以 '\0' 结尾） */
void GUI_ShowChineseStr(uint16_t x, uint16_t y, const uint8_t *str, FontSize fs, uint8_t color);

/* ================================================================
 * 混合显示（GBK 编码，ASCII + 汉字自动识别）
 *
 * 单字节 0x00..0x7F：视为 ASCII，使用内嵌点阵字体
 * 双字节 0x81..0xFE + 0x40..0xFE：视为 GBK 汉字，从 Flash 读字模
 *
 * 字符宽度（像素）：
 *   FONT_16X16 → ASCII 8px，汉字 16px
 *   FONT_24X24 → ASCII 12px，汉字 24px
 * ================================================================ */
void GUI_ShowText(uint16_t x, uint16_t y, const uint8_t *str, FontSize fs, uint8_t color);

#endif /* EPD_GUI_H */
