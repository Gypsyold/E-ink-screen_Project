#include "w25q128_test.h"
#include "w25q128.h"
#include "board_bsp.h"
#include "delay_bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ================================================================
 * 测试参数
 * ================================================================ */
#define TEST_ADDR   0x001000u   /* 测试地址（第 1 扇区，预留第 0 扇区给字库） */
#define TEST_LEN    256u        /* 测试长度（一整页）                          */

/* ================================================================
 * UART 输出辅助
 *
 * uart_print  — 发送固定字符串，无长度限制
 * uart_logfmt — 发送格式化短字符串，缓冲 128 字节
 *               中文 UTF-8 每字符 3 字节，注意勿超出缓冲
 * ================================================================ */
static void uart_print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
}

static void uart_logfmt(const char *fmt, ...)
{
    char buf[128];   /* 128 字节，可容纳约 40 个中文字符 */
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_print(buf);
}

/* ================================================================
 * 静态缓冲区（避免占用任务栈）
 * ================================================================ */
static uint8_t s_write_buf[TEST_LEN];
static uint8_t s_read_buf[TEST_LEN];

/* ================================================================
 * W25Q128_Test_Run
 * ================================================================ */
void W25Q128_Test_Run(void)
{
    uart_print("\r\n");
    uart_print("================================\r\n");
    uart_print("  W25Q128 Flash 驱动测试\r\n");
    uart_print("================================\r\n\r\n");

    /* ---- 步骤 1：读取并验证 JEDEC ID ---- */
    uart_print("[1] 读取 JEDEC ID...\r\n");
    uint32_t jedec = W25Q128_ReadJEDEC();
    uart_logfmt("    JEDEC ID = 0x%06lX\r\n", (unsigned long)jedec);

    if (jedec == W25Q128_JEDEC_ID) {
        uart_print("    [OK] 芯片识别正确（Winbond W25Q128）\r\n\r\n");
    } else {
        uart_print("    [FAIL] JEDEC ID 不匹配，测试中止\r\n");
        for (;;) { vTaskDelay(pdMS_TO_TICKS(1000)); }
    }

    /* ---- 步骤 2：擦除 + 页写入（DMA）---- */
    /* 拆成两行避免单条格式串过长 */
    uart_print("[2] 擦除 + 页写入（DMA）...\r\n");
    uart_logfmt("    地址: 0x%06lX  长度: %u 字节\r\n",
                (unsigned long)TEST_ADDR, (unsigned)TEST_LEN);

    for (uint16_t i = 0; i < TEST_LEN; i++) {
        s_write_buf[i] = (uint8_t)i;   /* 测试图案：0x00, 0x01, ..., 0xFF */
    }

    uint32_t t0 = Delay_BSP_Tick();
    W25Q128_Write(s_write_buf, TEST_ADDR, TEST_LEN);
    uint32_t t1 = Delay_BSP_Tick();

    uart_logfmt("    完成，用时 %lu ms\r\n", (unsigned long)(t1 - t0));
    uart_print("    （含扇区擦除约 50ms + DMA 写入约 1ms）\r\n\r\n");

    /* ---- 步骤 3：读取验证 ---- */
    uart_print("[3] 读取并校验...\r\n");
    memset(s_read_buf, 0, sizeof(s_read_buf));
    W25Q128_Read(s_read_buf, TEST_ADDR, TEST_LEN);

    uint16_t err_cnt = 0;
    uint16_t first_err_idx = 0;
    uint8_t  first_err_got = 0;
    uint8_t  first_err_exp = 0;
    for (uint16_t i = 0; i < TEST_LEN; i++) {
        if (s_read_buf[i] != s_write_buf[i]) {
            if (err_cnt == 0) {
                first_err_idx = i;
                first_err_got = s_read_buf[i];
                first_err_exp = s_write_buf[i];
            }
            err_cnt++;
        }
    }

    if (err_cnt == 0) {
        uart_logfmt("    [PASS] 全部 %u 字节数据一致 ✓\r\n", (unsigned)TEST_LEN);
    } else {
        uart_logfmt("    [FAIL] %u 字节不匹配\r\n", (unsigned)err_cnt);
        uart_logfmt("    首个错误: index[%u] 期望 0x%02X 实际 0x%02X\r\n",
                    (unsigned)first_err_idx,
                    (unsigned)first_err_exp,
                    (unsigned)first_err_got);
    }

    uart_print("\r\n================================\r\n");
    uart_print("  测试结束\r\n");
    uart_print("================================\r\n");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
