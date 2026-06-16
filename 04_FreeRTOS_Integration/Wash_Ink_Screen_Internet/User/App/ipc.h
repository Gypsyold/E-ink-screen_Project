#ifndef IPC_H
#define IPC_H

/*
 * ipc.h — 任务间通信对象
 *
 * 三个全局 IPC 句柄在 IPC_Init() 中创建：
 *   g_key_queue  : KeyTask → AppTask，传递按键事件
 *   g_epd_queue  : AppTask → EPDTask，传递 EPD 操作指令
 *   g_epd_done   : EPDTask → AppTask，EPD 操作完成信号
 *
 * 调用顺序：在 osKernelInitialize() 之后、osKernelStart() 之前调用 IPC_Init()
 */

#include "cmsis_os.h"
#include "key.h"

/* ------------------------------------------------------------------ */
/* 按键消息：一次按键事件对应一条消息                                   */
/* ------------------------------------------------------------------ */
typedef struct {
    KeyID    id;
    KeyEvent evt;
} KeyMsg_t;

/* ------------------------------------------------------------------ */
/* EPD 硬件指令                                                        */
/* ------------------------------------------------------------------ */
typedef enum {
    EPD_CMD_WAKE,       /* EPD_Init (硬件复位 + 控制器初始化)           */
    EPD_CMD_DISP_FULL,  /* EPD_Display + EPD_Update + EPD_DisplayPrev   */
    EPD_CMD_DISP_PART,  /* EPD_PartInit + Display + PartUpdate + Prev   */
    EPD_CMD_SLEEP,      /* EPD_DeepSleep                                */
} EPDCmdType_t;

typedef struct {
    EPDCmdType_t   type;
    const uint8_t *frame;  /* 帧缓冲区指针；WAKE / SLEEP 时为 NULL      */
} EPDCmd_t;

/* ------------------------------------------------------------------ */
/* 全局 IPC 句柄                                                       */
/* ------------------------------------------------------------------ */
extern osMessageQueueId_t g_key_queue;  /* KeyMsg_t，深度 8  */
extern osMessageQueueId_t g_epd_queue;  /* EPDCmd_t，深度 2  */
extern osSemaphoreId_t    g_epd_done;   /* 二值信号量         */
extern osMutexId_t        g_sd_mutex;   /* SD 卡互斥量：覆盖 Mount→操作→Unmount 会话 */

void IPC_Init(void);

#endif /* IPC_H */
