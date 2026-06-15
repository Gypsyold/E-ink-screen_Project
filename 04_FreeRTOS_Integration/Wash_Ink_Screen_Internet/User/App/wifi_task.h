#ifndef WIFI_TASK_H
#define WIFI_TASK_H

/*
 * wifi_task.h — WiFi模块文件接收任务
 *
 * 通过 USART2（PA2 TX / PA3 RX）与 ESP32-S3 通信，
 * 接收文件帧并写入 SD 卡 "0:/ebooks/" 目录。
 *
 * 帧格式（小端）：
 *   [0xAA][0x55][CMD][LEN_L][LEN_H][DATA × LEN]
 *
 * CMD 定义：
 *   0x01  FILENAME — DATA = 文件名字符串（不含路径前缀）
 *   0x02  DATA     — DATA = 文件内容片段，最大 512 字节
 *   0x03  EOF      — LEN = 0，通知文件传输完毕
 *
 * ACK（STM32 → ESP32）：
 *   [0xAA][0x55][0xAA]  成功
 *   [0xAA][0x55][0xFF]  失败
 */

void WiFi_TaskStart(void);

#endif /* WIFI_TASK_H */
