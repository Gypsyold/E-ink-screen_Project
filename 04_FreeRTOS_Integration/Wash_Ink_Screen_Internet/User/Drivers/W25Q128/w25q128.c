#include "w25q128.h"
#include "board_bsp.h"
#include "gpio_bsp.h"
#include "spi_bsp.h"
#include "delay_bsp.h"

/* ================================================================
 * SPI 命令字
 * ================================================================ */
#define CMD_WRITE_ENABLE   0x06u  /* 写使能             */
#define CMD_READ_SR1       0x05u  /* 读状态寄存器 1      */
#define CMD_READ_JEDEC     0x9Fu  /* 读 JEDEC ID         */
#define CMD_READ_DATA      0x03u  /* 读数据              */
#define CMD_PAGE_PROGRAM   0x02u  /* 页写入              */
#define CMD_SECTOR_ERASE   0x20u  /* 扇区擦除（4KB）     */
#define CMD_CHIP_ERASE     0xC7u  /* 整片擦除            */

#define SR1_WIP_BIT        0x01u  /* Status Register 1：WIP（写进行中）位 */

/* ================================================================
 * CS 片选操作
 * ================================================================ */
static inline void cs_low(void)  { GPIO_BSP_Clr(W25Q_CS_PORT, W25Q_CS_PIN); }
static inline void cs_high(void) { GPIO_BSP_Set(W25Q_CS_PORT, W25Q_CS_PIN); }

/* ================================================================
 * 内部辅助函数
 * ================================================================ */

/* 读取状态寄存器 1（SR1）*/
static uint8_t w25q128_read_sr1(void)
{
    uint8_t tx[2] = {CMD_READ_SR1, 0xFF};
    uint8_t rx[2] = {0, 0};
    cs_low();
    SPI_BSP_TransmitReceive(&hspi1, tx, rx, 2);
    cs_high();
    return rx[1];  /* rx[0] 是命令回环，rx[1] 才是 SR1 值 */
}

/* 等待内部操作完成（WIP 位清零）*/
static void w25q128_wait_busy(void)
{
    /* 扇区擦除最长约 400ms，整片擦除可达 200s
     * 每次轮询间隔 1ms，让出 CPU 给其他任务 */
    while (w25q128_read_sr1() & SR1_WIP_BIT) {
        Delay_BSP_ms(1);
    }
}

/* 发送写使能命令（必须在每次写 / 擦除前调用）*/
static void w25q128_write_enable(void)
{
    uint8_t cmd = CMD_WRITE_ENABLE;
    cs_low();
    SPI_BSP_Transmit(&hspi1, &cmd, 1);
    cs_high();
}

/* 页写入（内部函数）
 * - 调用前必须确保目标页所在扇区已擦除
 * - addr 可以非页对齐，但写入范围不得跨越页边界
 * - 数据段通过 DMA 发送（任务阻塞等待 DMA 完成）
 */
static void w25q128_write_page(const uint8_t *buf, uint32_t addr, uint16_t len)
{
    uint8_t cmd[4] = {
        CMD_PAGE_PROGRAM,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };

    w25q128_write_enable();
    w25q128_wait_busy();

    cs_low();
    SPI_BSP_Transmit(&hspi1, cmd, 4);               /* 命令+地址：阻塞发送 */
    SPI_BSP_TransmitDMA(&hspi1, buf, len);           /* 数据：DMA 发送，任务挂起等待 */
    cs_high();                                        /* DMA 完成后 CS 才拉高 */

    w25q128_wait_busy();                             /* 等待内部写入完成 */
}

/* ================================================================
 * 公开 API
 * ================================================================ */

uint32_t W25Q128_ReadJEDEC(void)
{
    uint8_t tx[4] = {CMD_READ_JEDEC, 0xFF, 0xFF, 0xFF};
    uint8_t rx[4] = {0};
    cs_low();
    SPI_BSP_TransmitReceive(&hspi1, tx, rx, 4);
    cs_high();
    /* rx[0] 是命令回环，rx[1..3] 是 JEDEC ID 的 3 个字节 */
    return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
}

/* 从 addr 读取 len 字节（阻塞，不使用 DMA）
 * SPI1 当前只配置了 TX DMA，RX 采用阻塞方式 */
void W25Q128_Read(uint8_t *buf, uint32_t addr, uint32_t len)
{
    uint8_t cmd[4] = {
        CMD_READ_DATA,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };
    cs_low();
    SPI_BSP_Transmit(&hspi1, cmd, 4);                /* 命令+地址 */
    SPI_BSP_Receive(&hspi1, buf, (uint16_t)len);     /* 读数据    */
    cs_high();
}

/* 擦除包含 addr 的 4KB 扇区（约 100ms）*/
void W25Q128_EraseSector(uint32_t addr)
{
    addr &= ~((uint32_t)(W25Q128_SECTOR_SIZE - 1));  /* 对齐到扇区边界 */

    uint8_t cmd[4] = {
        CMD_SECTOR_ERASE,
        (uint8_t)(addr >> 16),
        (uint8_t)(addr >> 8),
        (uint8_t)(addr)
    };

    w25q128_write_enable();
    w25q128_wait_busy();

    cs_low();
    SPI_BSP_Transmit(&hspi1, cmd, 4);
    cs_high();

    w25q128_wait_busy();   /* 等待扇区擦除完成 */
}

/* 整片擦除（约 30~200s，慎用）*/
void W25Q128_EraseChip(void)
{
    uint8_t cmd = CMD_CHIP_ERASE;
    w25q128_write_enable();
    w25q128_wait_busy();
    cs_low();
    SPI_BSP_Transmit(&hspi1, &cmd, 1);
    cs_high();
    w25q128_wait_busy();
}

/* 向 addr 写入 len 字节
 * - 自动擦除写入范围内的所有受影响扇区（原有数据会丢失）
 * - 按页拆分写入，每页数据段通过 DMA 发送
 * - 必须在 FreeRTOS 任务中调用
 */
void W25Q128_Write(uint8_t *buf, uint32_t addr, uint32_t len)
{
    if (buf == NULL || len == 0) return;

    /* 第一步：擦除写入范围内所有受影响扇区 */
    uint32_t sector_start = addr & ~((uint32_t)(W25Q128_SECTOR_SIZE - 1));
    uint32_t sector_end   = (addr + len - 1) & ~((uint32_t)(W25Q128_SECTOR_SIZE - 1));
    for (uint32_t s = sector_start; s <= sector_end; s += W25Q128_SECTOR_SIZE) {
        W25Q128_EraseSector(s);
    }

    /* 第二步：按页写入（每页最多 256 字节，DMA 发送数据段）*/
    while (len > 0) {
        /* 计算本次写入在当前页内的起始偏移和可写字节数 */
        uint16_t page_offset = (uint16_t)(addr & (W25Q128_PAGE_SIZE - 1));
        uint16_t write_len   = W25Q128_PAGE_SIZE - page_offset;
        if (write_len > (uint16_t)len) write_len = (uint16_t)len;

        w25q128_write_page(buf, addr, write_len);

        addr += write_len;
        buf  += write_len;
        len  -= write_len;
    }
}
