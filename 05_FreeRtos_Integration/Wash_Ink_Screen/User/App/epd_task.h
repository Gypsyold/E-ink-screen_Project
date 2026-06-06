#ifndef EPD_TASK_H
#define EPD_TASK_H

/*
 * epd_task.h — EPD 硬件操作任务
 *
 * 独占 SPI2 + DMA（EPD 外设）。
 * 从 g_epd_queue 接收 EPDCmd_t 指令，执行完毕后释放 g_epd_done。
 *
 * AppTask 发送指令后必须 osSemaphoreAcquire(g_epd_done) 等待完成，
 * 期间不得修改传递给 EPDTask 的帧缓冲区。
 */

void EPD_TaskStart(void);

#endif /* EPD_TASK_H */
