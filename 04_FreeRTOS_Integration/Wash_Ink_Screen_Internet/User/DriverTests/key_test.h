#ifndef KEY_TEST_H
#define KEY_TEST_H

/*
 * key_test.h — 按键驱动测试
 *
 * Key_Test_Run() 是一个无限循环，设计为在 FreeRTOS 任务中直接调用：
 *   void StartDefaultTask(void *argument) {
 *       Key_Test_Run();
 *   }
 *
 * 测试内容：
 *   每 10ms 调用 Key_Tick()，检测到短按 / 长按时通过串口打印信息。
 *   用串口助手（115200 baud）观察输出即可。
 */

void Key_Test_Run(void);

#endif /* KEY_TEST_H */
