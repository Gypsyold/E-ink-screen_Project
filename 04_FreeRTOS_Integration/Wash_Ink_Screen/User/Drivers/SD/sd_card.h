#ifndef SD_CARD_H
#define SD_CARD_H

/*
 * sd_card.h — SD 卡文件读取封装
 *
 * 基于 FATFS 中间件，提供简洁的文件读取接口。
 * 所有函数必须在 FreeRTOS 任务中调用（f_mount/f_read 内部会阻塞）。
 *
 * 典型调用流程：
 *   SD_Mount()  → SD_ReadFile() / SD_FileSize()  → SD_Unmount()
 *
 * 文件路径格式："0:/filename.txt"（根目录）
 */

#include <stdint.h>

/* ---- 返回值 ---- */
typedef enum {
    SD_OK              = 0,
    SD_ERR_NO_CARD,        /* 卡检测引脚为高，未插卡 */
    SD_ERR_MOUNT,          /* f_mount 失败（格式不对或读错误） */
    SD_ERR_NOT_FOUND,      /* 文件不存在 */
    SD_ERR_READ,           /* 读取过程中出错 */
} SD_Err;

/* 检测卡是否插入（读 PD3，低电平=已插入） */
uint8_t SD_IsDetected(void);

/* 挂载文件系统，mount=1 强制初始化卡 */
SD_Err  SD_Mount(void);

/* 卸载文件系统 */
void    SD_Unmount(void);

/* 读取整个文件到 buf
 *   buf_size : buf 的总字节数（函数内部留 1 字节写 '\0'）
 *   out_len  : 实际读到的字节数（不含 '\0'），可传 NULL
 */
SD_Err  SD_ReadFile(const char *path, uint8_t *buf,
                    uint32_t buf_size, uint32_t *out_len);

/* 获取文件大小（字节），不需要先挂载——f_stat 会自动处理 */
SD_Err  SD_FileSize(const char *path, uint32_t *out_size);

#endif /* SD_CARD_H */
