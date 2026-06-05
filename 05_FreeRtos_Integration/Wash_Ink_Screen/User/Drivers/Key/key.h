#ifndef KEY_H
#define KEY_H

/*
 * key.h — 按键驱动（消抖 + 单击 + 双击 + 长按）
 *
 * 使用方式：
 *   在 FreeRTOS 任务中每隔 10ms 调用一次 Key_Tick()，
 *   然后用 Key_GetEvent() 检查各按键是否产生了事件。
 *
 * 时序参数（1 tick = 10ms）：
 *   消抖窗口：30ms
 *   长按阈值：1500ms（按住超过后立即触发，不用等松开）
 *   双击窗口：500ms（第一次松开后等待第二次按下的最长时间）
 *
 * 注意：
 *   单击事件在第一次松开后最多延迟 500ms 才触发，
 *   因为驱动需要等待确认后续没有第二次按下。
 *   长按触发后不会再产生单击或双击事件。
 */

#include <stdint.h>

/* 按键 ID */
typedef enum {
    KEY1 = 0,
    KEY2,
    KEY3,
    KEY4,
    KEY_COUNT
} KeyID;

/* 按键事件 */
typedef enum {
    KEY_EVT_NONE   = 0, /* 无事件                          */
    KEY_EVT_SHORT,      /* 单击（第一次松开后延迟 500ms 触发） */
    KEY_EVT_DOUBLE,     /* 双击（第二次按下消抖通过后触发）   */
    KEY_EVT_LONG,       /* 长按（按住 1.5s 立即触发）         */
} KeyEvent;

void     Key_Tick(void);
KeyEvent Key_GetEvent(KeyID id);

#endif /* KEY_H */
