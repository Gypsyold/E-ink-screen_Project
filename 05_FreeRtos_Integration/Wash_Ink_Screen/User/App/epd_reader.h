#ifndef EPD_READER_H
#define EPD_READER_H

/*
 * epd_reader.h
 * SD 卡 GBK 文本文件 -> EPD 逐行显示
 *
 * 使用方式：
 *   EPD_SD_ShowFile("0:/display.txt");
 *
 * 文件格式要求：
 *   - GBK 编码（Windows 记事本另存为 ANSI）
 *   - 换行用 \r\n 或 \n
 *   - FONT_16X16：每行最多 18 个汉字 / 37 个 ASCII 字符
 *   - 最多显示 9 行（超出部分不显示）
 */

/* 读取 GBK 文本文件内容，逐行显示在 EPD */
void EPD_SD_ShowFile(const char *sd_path);

/* 扫描目录下所有文件，带序号显示在 EPD 和串口 */
void EPD_SD_ListFiles(const char *dir_path);

#endif /* EPD_READER_H */
