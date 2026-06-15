#include "wifi_task.h"
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

static void wifi_task_fn(void *arg)
{
    (void)arg;

    s_sem = xSemaphoreCreateBinary();
    HAL_UART_Receive_IT(&huart2, (uint8_t *)&s_rx1, 1u);   /* 启动 UART2 接收 */

    DBG_Print("[WiFi] Task ready, waiting frames...\r\n");

    uint8_t file_open  = 0u;
    uint8_t self_tested = 0u;  /* 上电自检标志，只跑一次 */

    for (;;) {
        /* 上电后第一次进入循环：发自检帧，验证 UART2 TX/RX 环回 */
        if (!self_tested) {
            self_tested = 1u;
            uint8_t st[] = {0xAAu, 0x55u, 0xFEu, 0x00u, 0x00u};
            HAL_UART_Transmit(&huart2, st, sizeof(st), 100u);
            DBG_Print("[WiFi] Self-test frame sent (AA 55 FE 00 00)\r\n");
        }

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
            /* 关闭上一次未完成的文件 */
            if (file_open) {
                f_close(&s_fp);
                SD_Unmount();
                file_open = 0u;
            }
            s_data_buf[len] = '\0';
            snprintf(s_filepath, sizeof(s_filepath),
                     "0:/ebooks/%s", (char *)s_data_buf);
            DBG_Fmt("[WiFi] New file: %s\r\n", s_filepath);

            if (SD_Mount() != SD_OK) {
                DBG_Print("[WiFi] SD mount fail\r\n");
                send_ack(ACK_FAIL);
                break;
            }
            if (f_open(&s_fp, s_filepath, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
                DBG_Print("[WiFi] f_open fail\r\n");
                SD_Unmount();
                send_ack(ACK_FAIL);
                break;
            }
            file_open = 1u;
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
                file_open = 0u;
                DBG_Fmt("[WiFi] Saved: %s\r\n", s_filepath);
                send_ack(ACK_OK);
            } else {
                send_ack(ACK_FAIL);
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
    .stack_size = 512u,   /* 栈：局部变量很少，大块数据都在 BSS */
    .priority   = osPriorityBelowNormal,
};

void WiFi_TaskStart(void)
{
    osThreadNew(wifi_task_fn, NULL, &wifi_task_attrs);
}
