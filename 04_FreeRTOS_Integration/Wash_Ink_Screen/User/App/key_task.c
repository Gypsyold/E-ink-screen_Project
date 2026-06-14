#include "key_task.h"
#include "ipc.h"
#include "key.h"
#include "cmsis_os.h"

static void key_task_fn(void *arg)
{
    (void)arg;
    for (;;) {
        Key_Tick();
        osDelay(10);

        /* 逐个检查所有按键，有事件则写入队列（非阻塞） */
        for (uint8_t i = 0u; i < (uint8_t)KEY_COUNT; i++) {
            KeyEvent evt = Key_GetEvent((KeyID)i);
            if (evt != KEY_EVT_NONE) {
                KeyMsg_t msg = { .id = (KeyID)i, .evt = evt };
                osMessageQueuePut(g_key_queue, &msg, 0u, 0u);
            }
        }
    }
}

static const osThreadAttr_t key_task_attrs = {
    .name       = "KeyTask",
    .stack_size = 256u,
    .priority   = osPriorityAboveNormal,
};

void Key_TaskStart(void)
{
    osThreadNew(key_task_fn, NULL, &key_task_attrs);
}
