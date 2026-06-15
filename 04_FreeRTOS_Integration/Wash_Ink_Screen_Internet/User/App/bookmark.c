#include "bookmark.h"
#include "w25q128.h"
#include <string.h>
#include <stdio.h>

/* ================================================================
 * 记录格式：64 字节对齐，50 条 = 3200B
 * ================================================================ */
typedef struct {
    char     path[60];    /* 文件完整路径，如 "0:/story.txt"，空槽首字节为 '\0' */
    uint32_t offset;      /* 上次退出时当前页的文件字节偏移 */
} BM_Record_t;            /* sizeof = 64 字节 */

/* ================================================================
 * 双缓冲扇区镜像格式（防止写入过程中断电导致数据整体损坏）：
 *   [Header 12B: magic + seq + crc32] + [Payload: 50 条记录 + 最近路径]
 * 每次提交时序号 +1、重算 CRC32，写到"非活动"的那个物理扇区，
 * 写完后再切换活动指针。万一本次写入中途断电，原活动扇区的数据
 * 完好如初；BM_Init 会在两份镜像中自动选出"校验通过且序号最新"的
 * 一份，不会再出现整块数据丢失的情况。
 * ================================================================ */
typedef struct {
    uint32_t magic;
    uint32_t seq;
    uint32_t crc;
} BM_Header_t;            /* sizeof = 12 字节 */

#define BM_MAGIC         0x314B4D42u   /* "BMK1"，标识本扇区为有效书签镜像 */
#define BM_HDR_SIZE      (sizeof(BM_Header_t))                   /* 12   */
#define BM_RECORDS_SIZE  (BM_MAX_RECORDS * sizeof(BM_Record_t))  /* 3200 */
#define BM_LAST_SIZE     60u
#define BM_PAYLOAD_SIZE  (BM_RECORDS_SIZE + BM_LAST_SIZE)        /* 3260 */
#define BM_IMAGE_SIZE    (BM_HDR_SIZE + BM_PAYLOAD_SIZE)         /* 3272，小于 4KB 扇区 */
#define BM_LAST_OFFSET   (BM_HDR_SIZE + BM_RECORDS_SIZE)         /* 3212：最近路径在缓冲区中的偏移 */

/* RAM 缓存：始终保存"当前活动镜像"的完整内容（含 Header），大小仍按一个扇区分配 */
static uint8_t s_buf[W25Q128_SECTOR_SIZE];

/* 当前活动镜像所在的物理扇区；下次提交会写入另一个扇区并切换 */
static uint32_t s_active_addr = BM_SECTOR_A;

/* 将缓冲区视为记录数组（跳过 Header）*/
static BM_Record_t *records(void)
{
    return (BM_Record_t *)(void *)(s_buf + BM_HDR_SIZE);
}

/* 标准 CRC32（多项式 0xEDB88320，逐位计算，无需查表），
 * 用于检测扇区数据是否因写入中途断电而被截断/损坏 */
static uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0u; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0u; bit < 8u; bit++) {
            crc = (crc & 1u) ? ((crc >> 1) ^ 0xEDB88320u) : (crc >> 1);
        }
    }
    return ~crc;
}

/* 校验当前 s_buf 中镜像的有效性（魔数 + CRC32），有效时输出其序号 */
static uint8_t image_valid(uint32_t *out_seq)
{
    const BM_Header_t *hdr = (const BM_Header_t *)(const void *)s_buf;
    if (hdr->magic != BM_MAGIC) return 0u;
    if (hdr->crc != crc32_calc(s_buf + BM_HDR_SIZE, BM_PAYLOAD_SIZE)) return 0u;
    *out_seq = hdr->seq;
    return 1u;
}

/* 提交 s_buf：序号 +1、重算 CRC32，写入"非活动"扇区后切换活动指针。
 * 即使本次写入中途断电，原活动扇区数据完好，下次 BM_Init 会自动回退使用它。 */
static void bm_commit(void)
{
    BM_Header_t *hdr    = (BM_Header_t *)(void *)s_buf;
    uint32_t     target = (s_active_addr == BM_SECTOR_A) ? BM_SECTOR_B : BM_SECTOR_A;

    hdr->magic = BM_MAGIC;
    hdr->seq   = hdr->seq + 1u;
    hdr->crc   = crc32_calc(s_buf + BM_HDR_SIZE, BM_PAYLOAD_SIZE);

    W25Q128_Write(s_buf, target, BM_IMAGE_SIZE);
    s_active_addr = target;
}

/* ================================================================
 * 公开 API
 * ================================================================ */

/* 启动时调用一次：从 A/B 两个物理扇区中选出"魔数与 CRC32 均有效、
 * 且序号更新"的一份载入 RAM 缓存；两份都无效（首次上电/均损坏）则建空表 */
void BM_Init(void)
{
    uint32_t seq_a = 0u, seq_b = 0u;
    uint8_t  valid_a, valid_b, a_wins;

    W25Q128_Read(s_buf, BM_SECTOR_A, BM_IMAGE_SIZE);
    valid_a = image_valid(&seq_a);

    W25Q128_Read(s_buf, BM_SECTOR_B, BM_IMAGE_SIZE);
    valid_b = image_valid(&seq_b);

    a_wins = (uint8_t)(valid_a && (!valid_b || seq_a >= seq_b));

    if (a_wins) {
        W25Q128_Read(s_buf, BM_SECTOR_A, BM_IMAGE_SIZE);  /* s_buf 当前是 B，重新载入 A */
        s_active_addr = BM_SECTOR_A;
    } else if (valid_b) {
        s_active_addr = BM_SECTOR_B;                       /* s_buf 中已是有效的 B 数据 */
    } else {
        memset(s_buf, 0, sizeof(s_buf));
        BM_Header_t *hdr = (BM_Header_t *)(void *)s_buf;
        hdr->magic = BM_MAGIC;
        hdr->seq   = 0u;
        hdr->crc   = crc32_calc(s_buf + BM_HDR_SIZE, BM_PAYLOAD_SIZE);
        s_active_addr = BM_SECTOR_A;   /* 占位：下次提交写入 B，使两份尽快都变为有效 */
    }
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

/* 更新或新增书签，并提交整块缓存（双缓冲写入，自动避免中断写坏）*/
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

    bm_commit();
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
        bm_commit();
    }
}

/* ================================================================
 * 最近文件路径（紧跟在记录表之后，60 字节）
 * ================================================================ */

/* 记录最近打开的文件路径；路径相同时跳过写入，减少 Flash 擦写次数 */
void BM_SetLast(const char *path)
{
    char *slot = (char *)(s_buf + BM_LAST_OFFSET);
    if ((uint8_t)slot[0] != 0xFFu && strncmp(slot, path, 59u) == 0) return;
    strncpy(slot, path, 59u);
    slot[59u] = '\0';
    bm_commit();
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
