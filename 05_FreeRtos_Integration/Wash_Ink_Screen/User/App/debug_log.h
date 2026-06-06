#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

/*
 * debug_log.h — 串口调试日志
 *
 * DBG_ENABLE = 1：启用独立 DBGTask（osPriorityLow），通过队列异步输出到 UART1。
 *               调用方非阻塞投递，满则丢弃，不影响主逻辑时序。
 * DBG_ENABLE = 0：所有宏展开为 ((void)0)，零代码零内存，编译期完全消除。
 *
 * 使用：
 *   main.c  RTOS_THREADS 区：调用 DBG_TaskStart() 一次
 *   任意任务：DBG_Print("xxx\r\n") / DBG_Fmt("val=%d\r\n", v)
 */

#define DBG_ENABLE  1   /* 1=开启调试输出，0=关闭 */

#if DBG_ENABLE
void DBG_TaskStart(void);
void DBG_Print(const char *s);
void DBG_Fmt(const char *fmt, ...);
#else
#define DBG_TaskStart()       ((void)0)
#define DBG_Print(s)          ((void)0)
#define DBG_Fmt(fmt, ...)     ((void)0)
#endif /* DBG_ENABLE */

#endif /* DEBUG_LOG_H */
