#ifndef BOOKMARK_H
#define BOOKMARK_H

/*
 * bookmark.h — 阅读书签（存储在 W25Q128 用户区，双缓冲防掉电损坏）
 *
 * Flash 布局：
 *   0x280000 / 0x281000 两个相邻 4KB 扇区轮流保存同一份数据镜像
 *   （Header: 魔数+序号+CRC32 + Payload: 最多 50 条书签记录 + 最近路径）。
 *   每次保存都写入"非活动"扇区并让序号 +1，写完再切换活动指针——
 *   这样即使写入过程中突然断电，另一份完好的旧数据依然有效，
 *   BM_Init 会自动选用"校验通过且序号最新"的一份，不会整体丢失。
 *   0x280000 以下为字库区，禁止写入。
 *
 * 使用方式：
 *   App_Task 启动时调用 BM_Init() 一次，自动选出有效且最新的镜像载入 RAM 缓存。
 *   打开文件时调用 BM_Load() 获取上次退出的字节偏移（0 = 无记录）。
 *   KEY4 退出文件时调用 BM_Save() 更新缓存并提交（双缓冲写入）。
 */

#include <stdint.h>

#define BM_SECTOR_A     0x280000u   /* 书签镜像扇区 A（双缓冲之一，用户区第 1 个扇区） */
#define BM_SECTOR_B     0x281000u   /* 书签镜像扇区 B（双缓冲之一，用户区第 2 个扇区） */
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

/* 记录 / 读取最近打开的文件路径
 * 存储在书签镜像 Payload 区、记录表之后（60 字节）
 * 主页用于显示"继续阅读"及进度 */
void     BM_SetLast(const char *path);
void     BM_GetLast(char *out, uint8_t out_size);

#endif /* BOOKMARK_H */
