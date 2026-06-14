#include "key.h"
#include "board_bsp.h"
#include "gpio_bsp.h"

/* ================================================================
 * 时序参数（单位：tick，1 tick = Key_Tick 调用周期 = 10ms）
 * ================================================================ */
#define DEBOUNCE_TICKS    3u    /* 消抖：连续 3 tick 同电平才确认 = 30ms  */
#define LONG_PRESS_TICKS  150u  /* 长按阈值：150 tick = 1500ms             */
#define DOUBLE_WIN_TICKS  50u   /* 双击窗口：50 tick = 500ms               */

/* ================================================================
 * 内部状态机
 *
 * 状态转移图：
 *
 *  IDLE
 *   │ 检测到按下
 *   ▼
 *  DEBOUNCE ──(消抖失败)──► IDLE
 *   │ 消抖通过
 *   ▼
 *  HELD ──(达到长按阈值)──► LONG_FIRED ──(松开)──► IDLE
 *   │ 在长按前松开
 *   ▼
 *  WAIT_DOUBLE ──(500ms 超时无第二次)──► 触发单击 ──► IDLE
 *   │ 检测到第二次按下
 *   ▼
 *  DEBOUNCE2 ──(消抖失败/抖动)──► 触发单击 ──► IDLE
 *   │ 消抖通过
 *   ▼ 触发双击
 *  HELD2 ──(松开)──► IDLE
 *
 * ================================================================ */
typedef enum {
    KS_IDLE,         /* 空闲，等待按下                   */
    KS_DEBOUNCE,     /* 第一次按下，消抖中               */
    KS_HELD,         /* 第一次按下确认，计时             */
    KS_LONG_FIRED,   /* 长按已触发，等待松开             */
    KS_WAIT_DOUBLE,  /* 第一次松开，等待双击第二次按下   */
    KS_DEBOUNCE2,    /* 第二次按下，消抖中               */
    KS_HELD2,        /* 双击已触发，等待第二次松开       */
} KeyState;

typedef struct {
    KeyState state;
    uint16_t debounce_cnt;    /* 消抖计数     */
    uint16_t hold_cnt;        /* 按住时长     */
    uint16_t double_wait_cnt; /* 双击等待计数 */
    KeyEvent event;           /* 待读取的事件 */
} KeyCtx;

static KeyCtx s_keys[KEY_COUNT];

/* ================================================================
 * 硬件引脚映射
 * ================================================================ */
static GPIO_TypeDef * const KEY_PORTS[KEY_COUNT] = {
    KEY1_PORT, KEY2_PORT, KEY3_PORT, KEY4_PORT
};
static const uint16_t KEY_PINS[KEY_COUNT] = {
    KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN
};

/* 读取原始电平：按下返回 1，未按返回 0 */
static uint8_t key_raw(KeyID id)
{
    return GPIO_BSP_Read(KEY_PORTS[id], KEY_PINS[id]) == KEY_ACTIVE ? 1u : 0u;
}

/* ================================================================
 * Key_Tick — 每 10ms 调用一次，更新全部按键状态
 * ================================================================ */
void Key_Tick(void)
{
    for (uint8_t i = 0; i < KEY_COUNT; i++) {
        KeyCtx *k = &s_keys[i];
        uint8_t  pressed = key_raw((KeyID)i);

        switch (k->state) {

        /* ---- 空闲 ---- */
        case KS_IDLE:
            if (pressed) {
                k->debounce_cnt = 1;
                k->state = KS_DEBOUNCE;
            }
            break;

        /* ---- 第一次按下消抖 ---- */
        case KS_DEBOUNCE:
            if (pressed) {
                k->debounce_cnt++;
                if (k->debounce_cnt >= DEBOUNCE_TICKS) {
                    k->hold_cnt = 0;
                    k->state = KS_HELD;
                }
            } else {
                /* 抖动噪声，重回空闲 */
                k->state = KS_IDLE;
            }
            break;

        /* ---- 第一次按下确认，计时 ---- */
        case KS_HELD:
            if (!pressed) {
                /* 在长按阈值前松开，进入双击等待窗口 */
                k->double_wait_cnt = 0;
                k->state = KS_WAIT_DOUBLE;
            } else {
                k->hold_cnt++;
                if (k->hold_cnt >= LONG_PRESS_TICKS) {
                    /* 达到长按阈值，立即触发 */
                    k->event = KEY_EVT_LONG;
                    k->state = KS_LONG_FIRED;
                }
            }
            break;

        /* ---- 长按已触发，等待松开 ---- */
        case KS_LONG_FIRED:
            if (!pressed) {
                k->state = KS_IDLE;
            }
            break;

        /* ---- 双击等待窗口 ---- */
        case KS_WAIT_DOUBLE:
            if (pressed) {
                /* 检测到第二次按下，进入第二次消抖 */
                k->debounce_cnt = 1;
                k->state = KS_DEBOUNCE2;
            } else {
                k->double_wait_cnt++;
                if (k->double_wait_cnt >= DOUBLE_WIN_TICKS) {
                    /* 超时无第二次按下 → 确认为单击 */
                    k->event = KEY_EVT_SHORT;
                    k->state = KS_IDLE;
                }
            }
            break;

        /* ---- 第二次按下消抖 ---- */
        case KS_DEBOUNCE2:
            if (pressed) {
                k->debounce_cnt++;
                if (k->debounce_cnt >= DEBOUNCE_TICKS) {
                    /* 第二次按下消抖通过 → 触发双击 */
                    k->event = KEY_EVT_DOUBLE;
                    k->state = KS_HELD2;
                }
            } else {
                /* 第二次按下时间太短（噪声），仍算单击 */
                k->event = KEY_EVT_SHORT;
                k->state = KS_IDLE;
            }
            break;

        /* ---- 双击已触发，等待第二次松开 ---- */
        case KS_HELD2:
            if (!pressed) {
                k->state = KS_IDLE;
            }
            break;

        default:
            k->state = KS_IDLE;
            break;
        }
    }
}

/* ================================================================
 * Key_GetEvent — 读取事件并清零
 * ================================================================ */
KeyEvent Key_GetEvent(KeyID id)
{
    if (id >= KEY_COUNT) return KEY_EVT_NONE;
    KeyEvent evt = s_keys[id].event;
    s_keys[id].event = KEY_EVT_NONE;
    return evt;
}
