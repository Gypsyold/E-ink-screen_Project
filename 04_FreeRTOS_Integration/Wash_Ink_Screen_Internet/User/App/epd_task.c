#include "epd_task.h"
#include "ipc.h"
#include "epd.h"
#include "cmsis_os.h"

static void epd_task_fn(void *arg)
{
    (void)arg;
    EPDCmd_t cmd;

    for (;;) {
        /* 阻塞等待 AppTask 发来的 EPD 操作指令 */
        osMessageQueueGet(g_epd_queue, &cmd, 0u, osWaitForever);

        switch (cmd.type) {

        case EPD_CMD_WAKE:
            /* 硬件复位 + 控制器初始化，不刷屏 */
            EPD_Init();
            break;

        case EPD_CMD_DISP_FULL:
            /* 全刷：将帧缓冲推入 RAM0x24，全波形刷新，同步 RAM0x26 */
            EPD_Display(cmd.frame);
            EPD_Update();
            EPD_DisplayPrev(cmd.frame);
            break;

        case EPD_CMD_DISP_PART:
            /* 局刷：仅刷新与上一帧有差异的像素 */
            EPD_PartInit();
            EPD_Display(cmd.frame);
            EPD_PartUpdate();
            EPD_DisplayPrev(cmd.frame);
            break;

        case EPD_CMD_SLEEP:
            EPD_DeepSleep();
            break;

        default:
            break;
        }

        /* 通知 AppTask：本次 EPD 操作已完成，帧缓冲区可以再次使用 */
        osSemaphoreRelease(g_epd_done);
    }
}

static const osThreadAttr_t epd_task_attrs = {
    .name       = "EPDTask",
    .stack_size = 1024u,
    .priority   = osPriorityNormal,
};

void EPD_TaskStart(void)
{
    osThreadNew(epd_task_fn, NULL, &epd_task_attrs);
}
