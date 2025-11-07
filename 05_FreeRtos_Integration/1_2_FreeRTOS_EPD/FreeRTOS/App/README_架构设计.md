# FreeRTOS/App 模块架构设计

## ? 当前问题分析

### 1. **文件职责不清晰**
```
? Key_RTOS.h 混入了：
   - 按键硬件定义
   - 按键事件处理
   - 显示画面枚举（CurrentScreen）
   - 显示命令定义（DispCmd）
   
? Key_RTOS.c 职责太多：
   - 硬件GPIO操作
   - 按键状态机处理
   - 显示命令生成逻辑
   
? Display.c 依赖混乱：
   - 需要 g_MenuSel（全局变量跨模块访问）
```

### 2. **数据流向不清晰**
```
中断 → Key_Tick() → Key_ProcessTask() → Display_ProcessTask()
       ↓ 状态机       ↓ 事件转换        ↓ 界面更新
       KeyMsg        DispCmd          g_MenuSel
```

---

## ? 理想架构（分层设计）

```
┌─────────────────────────────────────────────────────────────┐
│                     应用逻辑层 (App Logic)                     │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │
│  │ 主菜单逻辑  │  │ 阅读模式    │  │ 日历模式    │           │
│  └─────────────┘  └─────────────┘  └─────────────┘           │
└──────────┬───────────────────┬───────────────────┬────────────┘
           │                   │                   │
┌──────────▼───────────────────▼───────────────────▼────────────┐
│                     状态管理层 (State Manager)                  │
│  - 当前画面状态（SCREEN_XXX）                                   │
│  - 各画面的局部状态（菜单选中、页码等）                           │
│  - 状态变更通知                                                  │
└──────────┬────────────────────────────────────────────────────┘
           │
┌──────────▼────────────────────────────────────────────────────┐
│                    UI控制层 (UI Controller)                     │
│  - 接收显示命令（DispCmd）                                      │
│  - 更新画面状态                                                 │
│  - 调用渲染函数                                                 │
└──────────┬────────────────────────────────────────────────────┘
           │
┌──────────▼────────────────────────────────────────────────────┐
│                   事件处理层 (Event Handler)                    │
│  - 按键事件 → 应用命令转换                                      │
│  - 场景识别（根据画面状态决定按键功能）                          │
└──────────┬────────────────────────────────────────────────────┘
           │
┌──────────▼────────────────────────────────────────────────────┐
│                   驱动层 (Driver)                              │
│  - 按键硬件操作                                                 │
│  - 按键状态机（单击/双击/长按）                                │
│  - 事件队列（KeyMsg）                                          │
└──────────┬────────────────────────────────────────────────────┘
           │
        ┌──▼──┐
        │硬件│
        └─────┘
```

---

## ? 推荐的文件组织结构

### **方案一：按功能模块划分**（推荐）

```
FreeRTOS/App/
├── Driver/                    # 驱动层
│   ├── KeyDriver.c/h         # 按键硬件操作（GPIO、读取）
│   └── EPDDriver.c/h         # 屏幕驱动封装
│
├── Middleware/                # 中间件层
│   ├── KeyFSM.c/h            # 按键状态机（单击/双击/长按识别）
│   └── EventQueue.c/h        # 事件队列管理
│
├── Control/                   # 控制层
│   ├── DisplayControl.c/h    # 显示控制器（接收命令→更新状态）
│   └── KeyMapper.c/h         # 按键映射（按键事件→应用命令）
│
├── State/                     # 状态层
│   ├── ScreenState.c/h       # 画面状态管理
│   └── AppState.c/h          # 应用状态（菜单选中项、页码等）
│
└── Application/               # 应用层
    ├── MainMenu.c/h          # 主菜单逻辑
    ├── ReadingMode.c/h       # 阅读模式
    ├── CalendarMode.c/h      # 日历模式
    └── MenuPic.h             # 图片资源
```

### **方案二：保持现有结构，优化分工**（渐进式改造）

```
FreeRTOS/App/
├── Key_RTOS.c/h
│   ┌─ 职责：按键硬件 + 状态机 + 原始事件（KeyMsg）
│   └─ 不包含：显示相关定义、应用逻辑
│
├── Display.c/h
│   ┌─ 职责：显示控制 + 画面渲染
│   └─ 不包含：全局状态变量（改用状态管理模块）
│
├── Screen_state.c/h
│   ┌─ 职责扩展：完整的状态管理
│   │  - CurrentScreen（当前画面）
│   │  - g_MenuSel（菜单选中）
│   │  - g_ReadingPage（页码）
│   │  - 提供 Get/Set 接口
│   └─
│
├── AppCommand.h              【新增】
│   ┌─ 职责：应用层命令定义
│   └─ DispCmdType, DispCmd 等
│
└── KeyMapper.c/h             【新增】
    ┌─ 职责：按键事件→应用命令映射
    └─ 处理 KeyMsg → DispCmd 转换
```

---

## ? 数据流向（理想情况）

### 1. **按键 → 显示流程**
```
硬件按键按下
   ↓
Key_Tick() [中断调用]
   ↓ 扫描硬件，运行状态机
发送 KeyMsg → key_queue
   ↓
Key_ProcessTask() [接收消息]
   ↓ 查询当前画面状态
   ↓ 根据"画面+按键+事件"生成命令
发送 DispCmd → xDispCmdQueue
   ↓
Display_ProcessTask() [接收命令]
   ↓ 更新应用状态
   ↓ 刷新画面显示
```

### 2. **命令类型定义位置**
```c
// ? 现在：放在 Key_RTOS.h 里（按键模块不应该知道显示命令）
// ? 应该：放在独立的 AppCommand.h 或 Display.h

#ifndef _APP_COMMAND_H_
#define _APP_COMMAND_H_

#include "Screen_state.h"  // 只依赖画面状态

typedef enum {
    DISP_CMD_NONE,
    DISP_CMD_MENU_SEL_CHANGE,
    DISP_CMD_ENTER_CALENDAR,
    // ... 其他命令
} DispCmdType;

typedef struct {
    DispCmdType cmd;
    int16_t param;
} DispCmd;

#endif
```

---

## ? 重构建议（优先级排序）

### **高优先级：立即改进**

1. **分离定义文件** ?
   ```
   Key_RTOS.h 只保留按键相关
   创建 AppCommand.h 存放 DispCmd 定义
   创建 CommonType.h 存放 CurrentScreen 等共用类型
   ```

2. **规范化状态管理** ?
   ```
   所有全局状态都放到 ScreenState 模块
   - g_MenuSel 移到 Screen_state.c
   - g_ReadingPage 移到 Screen_state.c
   提供统一的 Get/Set 接口
   ```

3. **移除跨模块直接访问** ?
   ```
   Key_RTOS.c 不再直接 #include "Display.h"
   通过 ScreenState_Get() 查询当前状态
   ```

### **中优先级：逐步优化**

4. **拆分 Key_RTOS.c** ?
   ```
   KeyDriver.c       - GPIO操作、硬件读取
   KeyFSM.c          - 状态机逻辑
   KeyMapper.c       - 事件映射（KeyMsg → DispCmd）
   ```

5. **引入状态通知机制** ?
   ```
   当状态变化时，通知相关模块
   而不是让其他模块轮询
   ```

### **低优先级：长期规划**

6. **完整的应用分层** ?
   ```
   每个功能模块独立
   主菜单、阅读、日历各自维护状态
   通过消息传递通信
   ```

---

## ? 具体改造示例

### **示例1：分离 CurrentScreen 定义**

```c
// Common/ScreenType.h 【新建】
#ifndef _SCREEN_TYPE_H_
#define _SCREEN_TYPE_H_

typedef enum {
    SCREEN_MAIN_MENU,
    SCREEN_READING_SEL,
    SCREEN_READING_VIEW,
    SCREEN_CALENDAR
} CurrentScreen;

#endif
```

### **示例2：扩展 ScreenState 管理所有应用状态**

```c
// Screen_state.c
typedef struct {
    CurrentScreen screen;     // 当前画面
    uint8_t menuSel;          // 菜单选中（0=日历，1=阅读）
    uint16_t readingPage;     // 阅读页码
    // 可以继续添加其他状态
} AppState_t;

static AppState_t g_AppState = {
    .screen = SCREEN_MAIN_MENU,
    .menuSel = 0,
    .readingPage = 1
};

uint8_t AppState_GetMenuSel(void) {
    return g_AppState.menuSel;
}

void AppState_SetMenuSel(uint8_t sel) {
    g_AppState.menuSel = sel;
}
```

### **示例3：KeyMapper 模块分离**

```c
// KeyMapper.c 【新建】
#include "Key_RTOS.h"
#include "AppCommand.h"
#include "Screen_state.h"

void KeyMapper_Process(KeyMsg* msg, DispCmd* cmd) {
    CurrentScreen screen = ScreenState_Get();
    
    cmd->cmd = DISP_CMD_NONE;
    cmd->param = 0;
    
    switch (screen) {
        case SCREEN_MAIN_MENU:
            // 主菜单按键映射
            if (msg->id == KEY1 && msg->event & KEY_EVENT_SINGLE) {
                cmd->cmd = DISP_CMD_MENU_SEL_CHANGE;
                cmd->param = 0;
            }
            // ...
            break;
    }
}
```

---

## ? 检查清单

### 代码质量检查
- [ ] 每个 .h 文件职责单一
- [ ] 没有跨模块的直接变量访问（都用接口函数）
- [ ] 依赖关系清晰（上层依赖下层）
- [ ] 全局变量集中管理
- [ ] 模块间通过消息队列通信

### 当前状态
- [x] ? Key_RTOS.h 包含 Display.h（临时方案）
- [x] ? g_MenuSel 是全局变量（临时方案）
- [ ] ? 缺少完整的状态管理机制
- [ ] ? Key_RTOS.c 职责过多
- [ ] ? EPD_RTOS 文件未使用

---

## ? 总结

**核心原则：**
1. **单一职责** - 每个文件只做一件事
2. **分层清晰** - 硬件→驱动→中间件→应用
3. **接口隔离** - 模块间通过接口通信
4. **状态集中** - 所有状态统一管理

**改进步骤：**
1. 先分离定义文件（最容易）
2. 再统一状态管理（最重要）
3. 最后拆分大文件（长期优化）

建议先做 **方案二（渐进式改造）**，可以逐步改进而不影响现有功能！

