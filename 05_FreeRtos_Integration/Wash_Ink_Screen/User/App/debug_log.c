#include "debug_log.h"

#if DBG_ENABLE

#include "board_bsp.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/* 单条日志最大字节数（含 '\0'）；超长内容会被 vsnprintf 截断 */
#define DBG_MSG_LEN     64u
/* 队列深度：最多缓存 16 条；若 DBGTask 来不及消费则丢弃新消息 */
#define DBG_QUEUE_DEPTH  16u

static osMessageQueueId_t s_dbg_queue;

static void dbg_task_fn(void *arg)
{
    (void)arg;
    char buf[DBG_MSG_LEN];
    for (;;) {
        if (osMessageQueueGet(s_dbg_queue, buf, NULL, osWaitForever) == osOK) {
            HAL_UART_Transmit(&huart1, (uint8_t *)buf, (uint16_t)strlen(buf), HAL_MAX_DELAY);
        }
    }
}

void DBG_TaskStart(void)
{
    s_dbg_queue = osMessageQueueNew(DBG_QUEUE_DEPTH, DBG_MSG_LEN, NULL);

    static const osThreadAttr_t attr = {
        .name       = "DBGTask",
        .stack_size = 256u,
        .priority   = osPriorityLow,   /* 最低优先级，不抢占任何业务任务 */
    };
    osThreadNew(dbg_task_fn, NULL, &attr);
}

void DBG_Print(const char *s)
{
    char buf[DBG_MSG_LEN];
    strncpy(buf, s, DBG_MSG_LEN - 1u);
    buf[DBG_MSG_LEN - 1u] = '\0';
    osMessageQueuePut(s_dbg_queue, buf, 0u, 0u);   /* 非阻塞投递，满则丢弃 */
}

void DBG_Fmt(const char *fmt, ...)
{
    char buf[DBG_MSG_LEN];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    osMessageQueuePut(s_dbg_queue, buf, 0u, 0u);
}

#endif /* DBG_ENABLE */
