#ifndef KEY_TASK_H
#define KEY_TASK_H

/*
 * key_task.h — 按键扫描任务
 *
 * 以 osPriorityAboveNormal 运行，每 10ms 调用一次 Key_Tick()。
 * 检测到事件后将 KeyMsg_t 写入 g_key_queue，供 AppTask 消费。
 *
 * 好处：即使 AppTask 被 EPD 刷屏阻塞（最长 ~1s），按键事件
 *       也能被及时捕获，不会丢失。
 */

void Key_TaskStart(void);

#endif /* KEY_TASK_H */
