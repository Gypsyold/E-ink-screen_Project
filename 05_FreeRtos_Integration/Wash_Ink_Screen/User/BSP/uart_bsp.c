#include "uart_bsp.h"
#include "board_bsp.h"

/*
 * __io_putchar — printf 底层输出钩子
 *
 * syscalls.c 中 _write 逐字节调用此函数，
 * 这里把每个字符通过 HAL_UART_Transmit 发送到 UART1。
 * 阻塞发送，超时 1000ms（正常情况下单字节发送远小于 1ms）。
 */
int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    HAL_UART_Transmit(&huart1, &c, 1, 1000);
    return ch;
}
