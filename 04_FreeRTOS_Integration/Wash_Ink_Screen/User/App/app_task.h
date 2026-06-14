#ifndef APP_TASK_H
#define APP_TASK_H

/*
 * app_task.h — 应用主任务
 *
 * 职责：
 *   - 扫描 SD 卡文件列表，显示文件浏览器
 *   - 分页读取文件内容，显示在 EPD
 *   - 从 g_key_queue 读取按键事件（由 KeyTask 投递）
 *   - 将渲染好的帧缓冲通过 g_epd_queue 发给 EPDTask
 *   - 直接调用 W25Q128 字库读取（单用户无需额外同步）
 *   - 直接调用 FatFS SD 卡接口
 *
 * 由 main.c 的 StartDefaultTask 调用。
 */

void App_Task(void *arg);

#endif /* APP_TASK_H */
