#ifndef UART_BSP_H
#define UART_BSP_H

/*
 * uart_bsp.h — UART1 调试输出层
 *
 * 实现 __io_putchar，将 printf / puts 等标准 C 输出重定向到 UART1。
 * syscalls.c 中的 _write 以 __io_putchar 为弱符号钩子，
 * 本文件提供强符号实现，覆盖弱符号即可让 printf 正常工作。
 *
 * 无需手动调用本文件的任何函数，include 进编译单元即可生效。
 */

/* 无对外 API，printf 重定向自动生效 */

#endif /* UART_BSP_H */
