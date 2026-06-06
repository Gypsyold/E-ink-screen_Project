#include "bookmark.h"
#include "w25q128.h"
#include <string.h>
#include <stdio.h>

/* ================================================================
 * 记录格式：64 字节对齐，刚好 50 条 = 3200B < 4KB 扇区
 * ================================================================ */
typedef struct {
    char     path[60];    /* 文件完整路径，如 "0:/story.txt"，空槽首字节为 '\0' */
    uint32_t offset;      /* 上次退出时当前页的文件字节偏移 */
} BM_Record_t;            /* sizeof = 64 字节 */

/* 4KB 扇区缓存（BM_Init 时从 Flash 读入，BM_Save 时写回）*/
static uint8_t s_buf[W25Q128_SECTOR_SIZE];

/* 将缓冲区视为记录数组 */
static BM_Record_t *records(void)
{
    return (BM_Record_t *)(void *)s_buf;
}

/* ================================================================
 * 公开 API
 * ================================================================ */

/* 从 Flash 读取整个书签扇区到 RAM 缓存，App_Task 启动时调用一次 */
void BM_Init(void)
{
    W25Q128_Read(s_buf, BM_FLASH_ADDR, W25Q128_SECTOR_SIZE);
}

/* 判断槽位是否为空（Flash 擦除后全为 0xFF；写 '\0' 表示已清除）*/
static uint8_t slot_empty(const BM_Record_t *r)
{
    return (r->path[0] == '\0' || (uint8_t)r->path[0] == 0xFFu);
}

/* 按路径查找书签，返回上次退出时的字节偏移；未找到返回 0 */
uint32_t BM_Load(const char *path)
{
    BM_Record_t *recs = records();
    for (uint8_t i = 0u; i < BM_MAX_RECORDS; i++) {
        if (!slot_empty(&recs[i]) && strncmp(recs[i].path, path, 59u) == 0) {
            return recs[i].offset;
        }
    }
    return 0u;
}

/* 更新或新增书签，同时将整块缓存写回 Flash（自动擦除扇区）*/
void BM_Save(const char *path, uint32_t offset)
{
    BM_Record_t *recs   = records();
    int8_t       target = -1;
    int8_t       empty  = -1;

    for (uint8_t i = 0u; i < BM_MAX_RECORDS; i++) {
        if (slot_empty(&recs[i])) {
            if (empty < 0) empty = (int8_t)i;
            continue;   /* 空槽不参与路径比较 */
        }
        if (strncmp(recs[i].path, path, 59u) == 0) {
            target = (int8_t)i;
            break;
        }
    }

    if (target < 0) target = empty;    /* 新文件：使用空槽 */
    if (target < 0) return;            /* 表已满，放弃写入 */

    strncpy(recs[target].path, path, 59u);
    recs[target].path[59u] = '\0';
    recs[target].offset    = offset;

    /* W25Q128_Write 内部自动擦除受影响扇区再写入 */
    W25Q128_Write(s_buf, BM_FLASH_ADDR, W25Q128_SECTOR_SIZE);
}

/* 扫描书签表，清除已不在 SD 文件列表中的孤立条目 */
void BM_Sweep(const char *dir_prefix,
              const char *names_flat,
              uint8_t     name_stride,
              uint8_t     count)
{
    if (count == 0u) return;   /* SD 读取异常时不清扫，避免误删所有书签 */

    BM_Record_t *recs    = records();
    uint8_t      changed = 0u;
    char         expected[60u];

    for (uint8_t i = 0u; i < BM_MAX_RECORDS; i++) {
        if (slot_empty(&recs[i])) continue;

        uint8_t found = 0u;
        for (uint8_t j = 0u; j < count; j++) {
            const char *name = names_flat + (uint16_t)j * name_stride;
            snprintf(expected, sizeof(expected), "%s%s", dir_prefix, name);
            if (strncmp(recs[i].path, expected, 59u) == 0) {
                found = 1u;
                break;
            }
        }

        if (!found) {
            memset(&recs[i], 0, sizeof(BM_Record_t));
            changed = 1u;
        }
    }

    if (changed) {
        W25Q128_Write(s_buf, BM_FLASH_ADDR, W25Q128_SECTOR_SIZE);
    }
}

/* ================================================================
 * 最近文件路径（扇区内 offset 3200 处，50×64B = 3200B 之后的空闲区）
 * ================================================================ */
#define BM_LAST_OFFSET  3200u   /* 50 条记录 × 64B = 3200B，其后存最近路径 */

/* 记录最近打开的文件路径；路径相同时跳过写入，减少 Flash 擦写次数 */
void BM_SetLast(const char *path)
{
    char *slot = (char *)(s_buf + BM_LAST_OFFSET);
    if ((uint8_t)slot[0] != 0xFFu && strncmp(slot, path, 59u) == 0) return;
    strncpy(slot, path, 59u);
    slot[59u] = '\0';
    W25Q128_Write(s_buf, BM_FLASH_ADDR, W25Q128_SECTOR_SIZE);
}

/* 读取最近打开的文件路径；Flash 未写过时输出空字符串 */
void BM_GetLast(char *out, uint8_t out_size)
{
    const char *slot = (const char *)(s_buf + BM_LAST_OFFSET);
    if (slot[0] == '\0' || (uint8_t)slot[0] == 0xFFu) {
        out[0] = '\0';
        return;
    }
    strncpy(out, slot, (size_t)(out_size - 1u));
    out[out_size - 1u] = '\0';
}
