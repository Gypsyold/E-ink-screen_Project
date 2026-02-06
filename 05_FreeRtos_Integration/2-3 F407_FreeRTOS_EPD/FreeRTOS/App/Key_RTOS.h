#ifndef KEY_RTOS_H
#define KEY_RTOS_H

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"
#include "EPD_RTOS.h"

#define KEY_TIME_DOUBLE			500
#define KEY_TIME_LONG			2000
#define KEY_TIME_REPEAT			100

#define KEY_COUNT	4

// 按键ID（支持扩展）
typedef enum 
{
    KEY1,
    KEY2,
    KEY3,
    KEY4,
    KEY_MAX
} KeyID;


typedef enum 
{
    KEY_STATE_IDLE,            // 0：空闲状态（初始状态）
    KEY_STATE_WAIT_HOLD_OR_UP, // 1：等待长按判定或松开（首次按下后）
    KEY_STATE_WAIT_DOUBLE,     // 2：等待双击（首次松开后）
    KEY_STATE_WAIT_DOUBLE_UP,  // 3：等待双击后松开
    KEY_STATE_LONG_REPEAT      // 4：长按重复触发状态
} KeyState;


#define KEY_PRESSED				1		//按下
#define KEY_UNPRESSED			0		//未按下


// 按键事件标志（独立位，支持组合）
#define KEY_EVENT_HOLD       0x01    // 持续按住
#define KEY_EVENT_DOWN       0x02    // 按下瞬间
#define KEY_EVENT_UP         0x04    // 松开瞬间
#define KEY_EVENT_SINGLE     0x08    // 单击（松开后判定）
#define KEY_EVENT_DOUBLE     0x10    // 双击（间隔内两次按下）
#define KEY_EVENT_LONG       0x20    // 长按（超过阈值）
#define KEY_EVENT_REPEAT     0x40    // 长按重复触发



// 按键消息：包含ID和事件标志
typedef struct 
{
    KeyID id;                // 按键ID
    uint8_t event;           // 事件标志（可组合）
    uint32_t press_duration; // 按下持续时间（ms，仅UP/LONG事件有效）
} KeyMsg;




// 初始化按键模块
void Key_RTOS_Init(void);
void Key_ProcessTask(void *arg);
void Key_Tick(void);



// -------------------------- 显示画面状态定义 --------------------------
typedef enum 
{
    SCREEN_MAIN_MENU,       // 主菜单（初始画面）
    SCREEN_READING_SEL,     // 阅读模式-页码选择
    SCREEN_READING_VIEW,    // 阅读模式-内容显示
    SCREEN_CALENDAR         // 日历模式
} CurrentScreen;

// -------------------------- 显示命令定义 --------------------------
typedef enum 
{
    DISP_CMD_NONE,                  // 无命令
    // 主菜单命令
    DISP_CMD_MENU_SEL_CHANGE,       // 切换主菜单选中项（param=0:日历,1:阅读）
    DISP_CMD_ENTER_CALENDAR,        // 进入日历模式
    DISP_CMD_ENTER_READING_SEL,     // 进入阅读-页码选择
    // 阅读模式命令
    DISP_CMD_READING_PAGE_CHANGE,   // 切换页码（param=-1:上一页,1:下一页）
    DISP_CMD_ENTER_READING_VIEW,    // 进入阅读-内容显示
    DISP_CMD_BACK_TO_READING_SEL,   // 返回阅读-页码选择
    // 通用命令
    DISP_CMD_BACK_TO_MAIN,          // 返回主菜单
    DISP_CMD_REFRESH                // 刷新当前画面
} DispCmdType;

// 显示命令结构体（按键任务→显示任务）
typedef struct 
{
    DispCmdType cmd;        // 命令类型
    int16_t param;          // 附加参数（如页码偏移、选中项索引）
} DispCmd;




#endif

