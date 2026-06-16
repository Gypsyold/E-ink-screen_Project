#include "wifi_task.h"
#include "ipc.h"
#include "board_bsp.h"
#include "sd_card.h"
#include "fatfs.h"
#include "debug_log.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

/* ================================================================
 * UART2 环形缓冲区（ISR 写 / Task 读）
 * 大小必须是 2 的幂，方便位与代替取模
 * ================================================================ */
#define RING_SIZE  1024u
static volatile uint8_t  s_ring[RING_SIZE];
static volatile uint16_t s_head = 0u;   /* ISR 写指针 */
static volatile uint16_t s_tail = 0u;   /* Task 读指针 */
static volatile uint8_t  s_rx1;         /* HAL 单字节接收缓冲（DMA 不安全，用中断模式） */

static SemaphoreHandle_t s_sem = NULL;  /* 二值信号量：ISR 通知 Task 有数据 */

/* ================================================================
 * 帧协议常量
 * ================================================================ */
#define SOF0            0xAAu
#define SOF1            0x55u
#define CMD_FILENAME    0x01u
#define CMD_DATA        0x02u
#define CMD_EOF         0x03u
#define CMD_DELETE      0x04u
#define CMD_SCAN        0x05u
#define ACK_OK          0xAAu
#define ACK_FAIL        0xFFu
#define FRAME_DATA_MAX  512u

/* ================================================================
 * HAL UART RX 完成回调（覆盖 HAL 弱符号）
 * 每接收到 1 字节就将其压入环形缓冲区，然后重新挂起接收
 * ================================================================ */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) return;

    uint16_t next = (s_head + 1u) & (RING_SIZE - 1u);
    if (next != s_tail) {           /* 缓冲区未满才写入，满了丢弃 */
        s_ring[s_head] = s_rx1;
        s_head = next;
    }

    /* 重新挂起单字节接收 */
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&s_rx1, 1u);

    /* 通知 Task 有数据可读 */
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(s_sem, &woken);
    portYIELD_FROM_ISR(woken);
}

/* ================================================================
 * HAL UART 错误回调（覆盖 HAL 弱符号）
 * 高波特率下 FreeRTOS 临界区可能延迟 ISR，导致 ORE（接收溢出）。
 * HAL 默认实现不重启接收，必须在这里手动恢复，否则 UART 永久失聋。
 * ================================================================ */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) return;
    /* 清除错误标志并重新挂起接收 */
    __HAL_UART_CLEAR_OREFLAG(huart);
    HAL_UART_AbortReceive(huart);
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&s_rx1, 1u);
}

/* ================================================================
 * 内部工具
 * ================================================================ */
static inline uint8_t ring_avail(void)
{
    return (s_head != s_tail) ? 1u : 0u;
}

static inline uint8_t ring_pop(void)
{
    uint8_t b = s_ring[s_tail];
    s_tail = (s_tail + 1u) & (RING_SIZE - 1u);
    return b;
}

/*
 * 读取 n 个字节到 buf。
 * timeout_ms = 0 时永久阻塞；非零时若超时则返回 0（失败）。
 */
static uint8_t read_bytes(uint8_t *buf, uint16_t n, uint32_t timeout_ms)
{
    TickType_t start = xTaskGetTickCount();
    TickType_t total = pdMS_TO_TICKS(timeout_ms);

    for (uint16_t i = 0u; i < n; i++) {
        while (!ring_avail()) {
            if (timeout_ms != 0u) {
                TickType_t elapsed = xTaskGetTickCount() - start;
                if (elapsed >= total) return 0u;
                xSemaphoreTake(s_sem, total - elapsed);
            } else {
                xSemaphoreTake(s_sem, portMAX_DELAY);
            }
        }
        buf[i] = ring_pop();
    }
    return 1u;
}

static void send_ack(uint8_t result)
{
    uint8_t pkt[3] = {SOF0, SOF1, result};
    HAL_UART_Transmit(&huart2, pkt, 3u, 100u);
}

/* ================================================================
 * WiFi Task 主体
 * ================================================================ */
static uint8_t s_data_buf[FRAME_DATA_MAX];  /* BSS，不占任务栈 */
static FIL     s_fp;                         /* BSS，FIL 内含 512B 扇区缓冲 */
static char    s_filepath[80u];              /* BSS */
static DIR     s_scan_dir;                   /* BSS，SCAN 目录句柄 */
static FILINFO s_scan_fno;                   /* BSS，SCAN 文件信息（含 LFN） */
static uint8_t s_scan_buf[2048u];            /* BSS，SCAN 输出缓冲 */

static void wifi_task_fn(void *arg)
{
    (void)arg;

    s_sem = xSemaphoreCreateBinary();
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&s_rx1, 1u);   /* 启动 UART2 接收 */

    DBG_Print("[WiFi] Task ready, waiting frames...\r\n");

    uint8_t file_open    = 0u;  /* 1 = 正在下载（持有 g_sd_mutex） */

    for (;;) {
        /* ── 等待帧头第一字节 0xAA（永久阻塞）── */
        uint8_t b;
        read_bytes(&b, 1u, 0u);
        if (b != SOF0) continue;

        /* ── 帧头第二字节 0x55 ── */
        if (!read_bytes(&b, 1u, 1000u) || b != SOF1) continue;

        /* ── CMD ── */
        uint8_t cmd;
        if (!read_bytes(&cmd, 1u, 1000u)) { send_ack(ACK_FAIL); continue; }

        /* ── LEN（2 字节小端）── */
        uint8_t lb[2];
        if (!read_bytes(lb, 2u, 1000u)) { send_ack(ACK_FAIL); continue; }
        uint16_t len = (uint16_t)lb[0] | ((uint16_t)lb[1] << 8u);
        if (len > FRAME_DATA_MAX) { send_ack(ACK_FAIL); continue; }

        /* ── DATA ── */
        if (len > 0u && !read_bytes(s_data_buf, len, 3000u)) {
            send_ack(ACK_FAIL); continue;
        }

        /* ── 处理帧 ── */
        switch (cmd) {

        case CMD_FILENAME:
            /* 若上次未完成，先关闭（同时持有互斥量，一并释放）*/
            if (file_open) {
                f_close(&s_fp);
                SD_Unmount();
                osMutexRelease(g_sd_mutex);
                file_open = 0u;
            }
            s_data_buf[len] = '\0';
            snprintf(s_filepath, sizeof(s_filepath),
                     "0:/ebooks/%s", (char *)s_data_buf);
            DBG_Fmt("[WiFi] New file: %s\r\n", s_filepath);

            /* 下载会话开始：获取 SD 互斥量（最多等 5 秒）*/
            if (osMutexAcquire(g_sd_mutex, 5000u) != osOK) {
                DBG_Print("[WiFi] SD mutex timeout (FILENAME)\r\n");
                send_ack(ACK_FAIL);
                break;
            }
            if (SD_Mount() != SD_OK) {
                DBG_Print("[WiFi] SD mount fail\r\n");
                osMutexRelease(g_sd_mutex);
                send_ack(ACK_FAIL);
                break;
            }
            if (f_open(&s_fp, s_filepath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
                DBG_Print("[WiFi] f_open fail\r\n");
                SD_Unmount();
                osMutexRelease(g_sd_mutex);
                send_ack(ACK_FAIL);
                break;
            }
            file_open = 1u;   /* 互斥量保持到 CMD_EOF */
            send_ack(ACK_OK);
            break;

        case CMD_DATA:
            if (!file_open) { send_ack(ACK_FAIL); break; }
            {
                UINT bw;
                if (f_write(&s_fp, s_data_buf, len, &bw) != FR_OK || bw != len) {
                    DBG_Print("[WiFi] f_write fail\r\n");
                    send_ack(ACK_FAIL);
                } else {
                    send_ack(ACK_OK);
                }
            }
            break;

        case CMD_EOF:
            if (file_open) {
                f_sync(&s_fp);
                f_close(&s_fp);
                SD_Unmount();
                osMutexRelease(g_sd_mutex);   /* 下载会话结束，释放互斥量 */
                file_open = 0u;
                DBG_Fmt("[WiFi] Saved: %s\r\n", s_filepath);
                send_ack(ACK_OK);
            } else {
                send_ack(ACK_FAIL);
            }
            break;

        case CMD_SCAN:
            /* 扫描 /ebooks/ 目录
             * SD 忙时发 3 字节 ACK_FAIL（ESP32 协议已适配）；
             * SD 可用时发 5 字节扩展 ACK + 数据 */
            {
                uint16_t scan_len = 0u;
                /* 尝试获取互斥量；SD 正被读书任务使用时立即放弃，返回空扫描 */
                if (osMutexAcquire(g_sd_mutex, 50u) != osOK) {
                    DBG_Print("[WiFi] Scan: SD busy\r\n");
                    send_ack(ACK_FAIL);   /* 3 字节失败回复，ESP32 跳过本次同步 */
                    break;
                }
                if (SD_Mount() == SD_OK) {
                    if (f_opendir(&s_scan_dir, "0:/ebooks") == FR_OK) {
                        for (;;) {
                            if (f_readdir(&s_scan_dir, &s_scan_fno) != FR_OK) break;
                            if (s_scan_fno.fname[0] == '\0') break;
                            if (s_scan_fno.fattrib & AM_DIR) continue;
                            uint16_t nlen = (uint16_t)strlen(s_scan_fno.fname);
                            if (scan_len + nlen + 1u > sizeof(s_scan_buf)) break;
                            memcpy(s_scan_buf + scan_len, s_scan_fno.fname, nlen);
                            scan_len += nlen;
                            s_scan_buf[scan_len++] = '\0';
                        }
                        f_closedir(&s_scan_dir);
                    }
                    SD_Unmount();
                }
                osMutexRelease(g_sd_mutex);
                /* 扫描完成：发扩展 ACK */
                uint8_t scan_hdr[5] = {
                    SOF0, SOF1, ACK_OK,
                    (uint8_t)(scan_len & 0xFFu),
                    (uint8_t)(scan_len >> 8u),
                };
                HAL_UART_Transmit(&huart2, scan_hdr, sizeof(scan_hdr), 200u);
                if (scan_len > 0u) {
                    HAL_UART_Transmit(&huart2, s_scan_buf, scan_len, 2000u);
                }
                DBG_Fmt("[WiFi] Scan: %u bytes\r\n", (unsigned)scan_len);
            }
            break;

        case CMD_DELETE:
            /* 关闭正在写入的文件（保护：不应同时下载和删除，但做防御）*/
            if (file_open) {
                f_close(&s_fp);
                SD_Unmount();
                osMutexRelease(g_sd_mutex);
                file_open = 0u;
            }
            s_data_buf[len] = '\0';
            {
                char del_path[96];
                snprintf(del_path, sizeof(del_path),
                         "0:/ebooks/%s", (char *)s_data_buf);
                DBG_Fmt("[WiFi] Delete: %s\r\n", del_path);
                /* 等待 SD 空闲（最多 3 秒）*/
                if (osMutexAcquire(g_sd_mutex, 3000u) != osOK) {
                    DBG_Print("[WiFi] SD mutex timeout (DELETE)\r\n");
                    send_ack(ACK_FAIL);
                    break;
                }
                if (SD_Mount() != SD_OK) {
                    DBG_Print("[WiFi] SD mount fail (delete)\r\n");
                    osMutexRelease(g_sd_mutex);
                    send_ack(ACK_FAIL);
                    break;
                }
                FRESULT fr = f_unlink(del_path);
                SD_Unmount();
                osMutexRelease(g_sd_mutex);
                if (fr == FR_OK || fr == FR_NO_FILE) {
                    DBG_Fmt("[WiFi] Deleted OK (fr=%d)\r\n", (int)fr);
                    send_ack(ACK_OK);
                } else {
                    DBG_Fmt("[WiFi] f_unlink fail fr=%d\r\n", (int)fr);
                    send_ack(ACK_FAIL);
                }
            }
            break;

        default:
            DBG_Fmt("[WiFi] Unknown CMD=0x%02X\r\n", (unsigned)cmd);
            send_ack(ACK_FAIL);
            break;
        }
    }
}

static const osThreadAttr_t wifi_task_attrs = {
    .name       = "WiFiTask",
    .stack_size = 2048u,  /* f_readdir + LFN heap + FatFS 内部需要较大栈空间 */
    .priority   = osPriorityBelowNormal,
};

void WiFi_TaskStart(void)
{
    osThreadNew(wifi_task_fn, NULL, &wifi_task_attrs);
}
