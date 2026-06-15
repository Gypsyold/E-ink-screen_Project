#include "sd_test.h"
#include "sd_card.h"
#include "board_bsp.h"
#include "delay_bsp.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

/* ---- 测试用文件路径（SD 卡根目录下放一个 test.txt） ---- */
#define TEST_FILE_PATH   "0:/test.txt"
#define FILE_BUF_SIZE    512u

static uint8_t s_file_buf[FILE_BUF_SIZE];

/* ================================================================
 * 内部 UART 打印
 * ================================================================ */
static void uart_print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str,
                      (uint16_t)strlen(str), HAL_MAX_DELAY);
}

static void uart_logfmt(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    uart_print(buf);
}

/* ================================================================
 * SD_Test — SD 卡功能验证，在 FreeRTOS 任务中调用
 * ================================================================ */
void SD_Test(void)
{
    uart_print("\r\n================================\r\n");
    uart_print("  SD Card Test\r\n");
    uart_print("================================\r\n\r\n");

    /* ---- [1] 检测 ---- */
    uart_print("[1] 检测 SD 卡...\r\n");
    if (!SD_IsDetected()) {
        uart_print("    未检测到 SD 卡（PD3=HIGH），请插入后重试\r\n");
        return;
    }
    uart_print("    已检测到 SD 卡\r\n\r\n");

    /* ---- [2] 挂载 ---- */
    uart_print("[2] 挂载文件系统...\r\n");
    SD_Err err = SD_Mount();
    if (err != SD_OK) {
        uart_logfmt("    挂载失败，错误码 %d\r\n"
                    "    （确认 SD 卡已格式化为 FAT32）\r\n", (int)err);
        return;
    }
    uart_print("    挂载成功\r\n\r\n");

    /* ---- [3] 获取文件大小 ---- */
    uart_logfmt("[3] 查询文件：%s\r\n", TEST_FILE_PATH);
    uint32_t fsize = 0u;
    err = SD_FileSize(TEST_FILE_PATH, &fsize);
    if (err != SD_OK) {
        uart_print("    文件不存在，请在 SD 卡根目录放置 test.txt\r\n\r\n");
    } else {
        uart_logfmt("    文件大小：%lu 字节\r\n\r\n", (unsigned long)fsize);
    }

    /* ---- [4] 读取文件内容 ---- */
    uart_print("[4] 读取文件内容...\r\n");
    uint32_t read_len = 0u;
    err = SD_ReadFile(TEST_FILE_PATH, s_file_buf, FILE_BUF_SIZE, &read_len);
    switch (err) {
        case SD_OK:
            uart_logfmt("    读取成功，%lu 字节\r\n", (unsigned long)read_len);
            uart_print("    内容：\r\n----\r\n");
            uart_print((char *)s_file_buf);
            uart_print("\r\n----\r\n\r\n");
            break;
        case SD_ERR_NOT_FOUND:
            uart_print("    文件不存在\r\n\r\n");
            break;
        default:
            uart_logfmt("    读取失败，错误码 %d\r\n\r\n", (int)err);
            break;
    }

    /* ---- [5] 卸载 ---- */
    uart_print("[5] 卸载文件系统\r\n");
    SD_Unmount();
    uart_print("    完成\r\n\r\n");

    uart_print("================================\r\n");
    uart_print("  SD Card Test 完成\r\n");
    uart_print("================================\r\n");

    for (;;) {
        Delay_BSP_ms(1000);
    }
}
