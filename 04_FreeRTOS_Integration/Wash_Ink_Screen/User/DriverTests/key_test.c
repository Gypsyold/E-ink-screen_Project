#include "key_test.h"
#include "key.h"
#include "board_bsp.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>
#include <stdio.h>

static void uart_print(const char *str)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)str, (uint16_t)strlen(str), HAL_MAX_DELAY);
}

static void uart_printf(const char *fmt, const char *arg)
{
    char buf[32];
    snprintf(buf, sizeof(buf), fmt, arg);
    uart_print(buf);
}

static const char * const KEY_NAMES[KEY_COUNT] = {
    "KEY1", "KEY2", "KEY3", "KEY4"
};

void Key_Test_Run(void)
{
    uart_print("\r\n");
    uart_print("================================\r\n");
    uart_print("  系统启动，按键测试开始\r\n");
    uart_print("  单击：按下 < 1.5s 后松开\r\n");
    uart_print("  双击：快速按两下（< 500ms）\r\n");
    uart_print("  长按：按住 >= 1.5s\r\n");
    uart_print("  注意：单击延迟 500ms 后才显示\r\n");
    uart_print("================================\r\n\r\n");

    for (;;) {
        Key_Tick();

        for (KeyID id = KEY1; id < KEY_COUNT; id++) {
            KeyEvent evt = Key_GetEvent(id);
            if (evt == KEY_EVT_SHORT) {
                uart_printf("[单击] %s\r\n", KEY_NAMES[id]);
            } else if (evt == KEY_EVT_DOUBLE) {
                uart_printf("[双击] %s\r\n", KEY_NAMES[id]);
            } else if (evt == KEY_EVT_LONG) {
                uart_printf("[长按] %s\r\n", KEY_NAMES[id]);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
