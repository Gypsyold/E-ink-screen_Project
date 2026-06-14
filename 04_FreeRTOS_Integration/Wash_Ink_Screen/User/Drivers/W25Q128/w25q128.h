#ifndef W25Q128_H
#define W25Q128_H

/*
 * w25q128.h — W25Q128 SPI NOR Flash 驱动
 *
 * 依赖 BSP 层：board_bsp / gpio_bsp / spi_bsp / delay_bsp
 * 不直接调用任何 HAL 函数。
 *
 * SPI 策略：
 *   页写入（Page Program）：命令+地址 阻塞发送，数据段通过 DMA 发送
 *   读取（Read Data）      ：命令+地址 阻塞发送，数据段 阻塞接收
 *   其余控制命令           ：全部阻塞发送（字节数少，DMA 无收益）
 *
 * 调用约束：
 *   W25Q128_Write / W25Q128_Read 等函数必须在 FreeRTOS 任务中调用，
 *   因为页写入的 DMA 阶段会使任务挂起等待信号量。
 *
 * Flash 参数：
 *   总容量：128Mbit = 16MB
 *   扇区大小：4KB（最小擦除单位）
 *   页大小：256B（最大单次写入单位）
 *   JEDEC ID：0xEF4018（制造商 0xEF，型号 0x4018）
 */

#include <stdint.h>

#define W25Q128_SECTOR_SIZE   4096u       /* 4KB，扇区（最小擦除单位）  */
#define W25Q128_PAGE_SIZE     256u        /* 256B，页（最大单次写入）   */
#define W25Q128_TOTAL_SIZE    (16*1024*1024u)  /* 16MB                 */
#define W25Q128_JEDEC_ID      0xEF4018u   /* Winbond W25Q128 标识      */

/*
 * W25Q128_ReadJEDEC  — 读取 3 字节 JEDEC ID，返回值高字节 = 制造商ID
 *                       正常应返回 0xEF4018
 * W25Q128_Read       — 从 addr 读取 len 字节到 buf（阻塞）
 * W25Q128_Write      — 向 addr 写入 len 字节（自动擦除受影响扇区，
 *                       数据段使用 DMA，必须在任务中调用）
 * W25Q128_EraseSector — 擦除包含 addr 的 4KB 扇区（约 100ms）
 * W25Q128_EraseChip  — 整片擦除（约 30~200s，慎用）
 */
uint32_t W25Q128_ReadJEDEC(void);
void     W25Q128_Read(uint8_t *buf, uint32_t addr, uint32_t len);
void     W25Q128_Write(uint8_t *buf, uint32_t addr, uint32_t len);
void     W25Q128_EraseSector(uint32_t addr);
void     W25Q128_EraseChip(void);

#endif /* W25Q128_H */
