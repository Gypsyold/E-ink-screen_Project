#include "ipc.h"

osMessageQueueId_t g_key_queue;
osMessageQueueId_t g_epd_queue;
osSemaphoreId_t    g_epd_done;

void IPC_Init(void)
{
    g_key_queue = osMessageQueueNew(8u, sizeof(KeyMsg_t), NULL);
    g_epd_queue = osMessageQueueNew(2u, sizeof(EPDCmd_t), NULL);
    g_epd_done  = osSemaphoreNew(1u, 0u, NULL);
}
