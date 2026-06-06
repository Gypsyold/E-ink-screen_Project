#ifndef BOOKMARK_H
#define BOOKMARK_H

/*
 * bookmark.h — 阅读书签（存储在 W25Q128 用户区）
 *
 * Flash 布局：
 *   0x280000 起始的第一个 4KB 扇区，存储最多 50 条书签记录。
 *   每条记录 64 字节：文件路径(60B) + 字节偏移(4B)。
 *   0x280000 以下为字库区，禁止写入。
 *
 * 使用方式：
 *   App_Task 启动时调用 BM_Init() 一次，将扇区读入 RAM 缓存。
 *   打开文件时调用 BM_Load() 获取上次退出的字节偏移（0 = 无记录）。
 *   KEY4 退出文件时调用 BM_Save() 更新缓存并写回 Flash。
 */

#include <stdint.h>

#define BM_FLASH_ADDR   0x280000u   /* 书签扇区起始地址（用户区第一个扇区） */
#define BM_MAX_RECORDS  50u         /* 最多存储 50 个文件的书签 */

void     BM_Init(void);
uint32_t BM_Load(const char *path);
void     BM_Save(const char *path, uint32_t offset);

/* 清除书签表中不在当前 SD 文件列表中的孤立条目
 * dir_prefix : 目录前缀，如 "0:/"
 * names_flat : 文件名平铺数组（每项为不带前缀的文件名）
 * name_stride: 每项占用字节数（MAX_NAME）
 * count      : 文件数量（为 0 时跳过，防止 SD 读取异常时误清） */
void     BM_Sweep(const char *dir_prefix,
                  const char *names_flat,
                  uint8_t     name_stride,
                  uint8_t     count);

#endif /* BOOKMARK_H */
