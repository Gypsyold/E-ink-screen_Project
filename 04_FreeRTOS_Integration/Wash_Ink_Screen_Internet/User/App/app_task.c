#include "app_task.h"
#include "ipc.h"
#include "epd.h"
#include "epd_gui.h"
#include "sd_card.h"
#include "fatfs.h"
#include "board_bsp.h"
#include "delay_bsp.h"
#include "debug_log.h"
#include "bookmark.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdio.h>

/* SD 互斥量辅助宏（5 秒超时，失败时直接 return）*/
#define SD_LOCK()  do { \
    if (osMutexAcquire(g_sd_mutex, 5000u) != osOK) { \
        DBG_Print("[App] SD mutex timeout\r\n"); return; \
    } \
} while (0)
#define SD_UNLOCK() osMutexRelease(g_sd_mutex)

/* ================================================================
 * 常量
 * ================================================================ */
#define READER_BUF_SIZE   512u
#define LINE_H            16u
#define DISPLAY_W         296u
#define DISPLAY_H         152u
#define MAX_FILES         50u
#define MAX_NAME          64u
#define MAX_VISIBLE       9u    /* 文件浏览器：最多显示行数 */
#define PAGE_LINES        8u    /* 文件阅读器：内容行数（第9行为状态栏） */
#define MAX_PAGES         200u  /* 每个文件最多缓存的页偏移数（200×4B=800B BSS） */
#define FILE_EOF          0xFFFFFFFFu

/* ================================================================
 * 静态存储（全部在 AppTask 上下文中访问，无需互斥）
 * ================================================================ */
static uint8_t  s_frame[EPD_FRAME_BYTES];          /* EPD 帧缓冲区 */
static uint8_t  s_file_buf[READER_BUF_SIZE];       /* SD 卡读取缓冲 */
static char     s_filenames[MAX_FILES][MAX_NAME];  /* 文件名列表 */
static uint8_t  s_file_count;
static uint32_t s_page_offsets[MAX_PAGES];         /* 每页的文件字节偏移 */
static uint8_t  s_page_count;
static uint8_t  s_scan_mode;   /* 非零时 render_page_from_file 只计算页边界，跳过 EPD 渲染 */

/* ================================================================
 * EPD 操作封装
 * 将指令写入 g_epd_queue，然后阻塞等待 EPDTask 完成（g_epd_done）。
 * 在等待期间 KeyTask 仍正常运行，按键不会丢失。
 * ================================================================ */
static void epd_cmd(EPDCmdType_t type, const uint8_t *frame)
{
    EPDCmd_t cmd = { .type = type, .frame = frame };
    osMessageQueuePut(g_epd_queue, &cmd, 0u, osWaitForever);
    osSemaphoreAcquire(g_epd_done, osWaitForever);
}

/* EPD 唤醒：硬件复位 + 控制器初始化，同时重置画布 */
static void epd_wake(void)
{
    Canvas_Init(s_frame, 0u, GUI_WHITE);
    Canvas_Clear(GUI_WHITE);
    epd_cmd(EPD_CMD_WAKE, NULL);
}

/* ================================================================
 * 文件浏览器渲染
 * ================================================================ */
static void render_list(uint8_t selected, uint8_t view_start)
{
    Canvas_Clear(GUI_WHITE);
    for (uint8_t i = 0u; i < MAX_VISIBLE; i++) {
        uint8_t fi = view_start + i;
        if (fi >= s_file_count) break;
        char line[80];
        char prefix = (fi == selected) ? '>' : ' ';
        snprintf(line, sizeof(line), "%c[%d] %s", prefix, (int)(fi + 1u), s_filenames[fi]);
        GUI_ShowText(0u, (uint16_t)(i * LINE_H), (uint8_t *)line, FONT_16X16, GUI_BLACK);
    }
}

/* 全刷：首次显示 / 从文件返回后使用，同时同步 RAM0x26 */
static void draw_list_full(uint8_t selected, uint8_t view_start)
{
    render_list(selected, view_start);
    epd_cmd(EPD_CMD_DISP_FULL, s_frame);
}

/* 局刷：光标移动时使用，仅刷新有变化的像素（速度快） */
static void draw_list_part(uint8_t selected, uint8_t view_start)
{
    render_list(selected, view_start);
    epd_cmd(EPD_CMD_DISP_PART, s_frame);
}

/* ================================================================
 * 文件内容分页渲染
 *
 * 从文件 fp 的 start_offset 处开始，渲染最多 PAGE_LINES(8) 行到 s_frame。
 * 使用 512B 缓冲块读取，GBK/ASCII 自动识别，自动折行。
 *
 * 返回：下一页的文件字节偏移；FILE_EOF 表示已到最后一页。
 * ================================================================ */
static uint32_t render_page_from_file(FIL *fp, uint32_t start_offset)
{
    if (f_lseek(fp, start_offset) != FR_OK) return FILE_EOF;
    if (!s_scan_mode) {
        Canvas_Clear(GUI_WHITE);
    }

    UINT     br;
    uint16_t buf_pos   = 0u;
    uint16_t buf_len   = 0u;
    uint32_t base_fpos = start_offset; /* 最近一次 f_read 后的 f_tell 值 */
    uint8_t  at_eof    = 0u;

    uint16_t y       = 0u;
    uint8_t  lines   = 0u;
    uint8_t  seg[64];
    uint8_t  seg_len = 0u;
    uint16_t x       = 0u;

    /* 从缓冲块中取一个字节；缓冲用尽时重新读取 512B */
    #define RBUF_GET(dst, ok) do {                                   \
        if (buf_pos >= buf_len && !at_eof) {                         \
            f_read(fp, s_file_buf, READER_BUF_SIZE, &br);           \
            buf_len   = (uint16_t)br;                                \
            buf_pos   = 0u;                                          \
            base_fpos = f_tell(fp);                                  \
            if (br == 0u) at_eof = 1u;                              \
        }                                                            \
        if (buf_pos < buf_len) { (dst) = s_file_buf[buf_pos++]; (ok) = 1u; } \
        else                  { (dst) = 0u; (ok) = 0u; }           \
    } while (0)

    /* 将 seg 中累积的内容渲染到画布并推进行计数 */
    #define FLUSH_LINE() do {                                        \
        if (seg_len > 0u && lines < PAGE_LINES) {                   \
            seg[seg_len] = '\0';                                     \
            if (!s_scan_mode) {                                      \
                GUI_ShowText(0u, y, seg, FONT_16X16, GUI_BLACK);    \
                DBG_Fmt("L%d: %s\r\n", (int)lines, (char *)seg);   \
            }                                                        \
            y += LINE_H; lines++;                                    \
        }                                                            \
        seg_len = 0u; x = 0u;                                       \
    } while (0)

    while (lines < PAGE_LINES) {
        /* 记录即将读取的字符在文件中的偏移；
           若该字符触发分页，此偏移即为下一页起始位置 */
        uint32_t char_off = base_fpos - (uint32_t)buf_len + (uint32_t)buf_pos;

        uint8_t b, ok;
        RBUF_GET(b, ok);
        if (!ok) break;

        if (b == '\r') {
            FLUSH_LINE();
            uint8_t b2, ok2;
            RBUF_GET(b2, ok2);
            if (ok2 && b2 != '\n') buf_pos--;  /* 退回非换行字节 */
        } else if (b == '\n') {
            FLUSH_LINE();
        } else {
            uint8_t  ch_bytes = 1u;
            uint16_t ch_width = 8u;
            uint8_t  b2       = 0u;

            if (b >= 0x81u) {                  /* GBK 高字节 */
                uint8_t ok2;
                RBUF_GET(b2, ok2);
                if (ok2 && b2 >= 0x40u) {
                    ch_bytes = 2u;
                    ch_width = 16u;
                } else {
                    if (ok2) buf_pos--;         /* 无效第二字节，退回 */
                }
            }

            if (x + ch_width > DISPLAY_W) {    /* 行宽不足，自动折行 */
                FLUSH_LINE();
                if (lines >= PAGE_LINES) {
                    /* 该字符属于下一页，直接返回其文件偏移 */
                    return char_off;
                }
            }

            if (seg_len + ch_bytes < (uint8_t)sizeof(seg)) {
                seg[seg_len++] = b;
                if (ch_bytes == 2u) seg[seg_len++] = b2;
                x += ch_width;
            }
        }
    }

    FLUSH_LINE();   /* 刷出 EOF 或满页前最后一个不完整行 */

    #undef RBUF_GET
    #undef FLUSH_LINE

    if (at_eof) return FILE_EOF;

    /* 循环因 lines == PAGE_LINES 退出，buf_pos 指向下一页第一个字节 */
    return base_fpos - (uint32_t)buf_len + (uint32_t)buf_pos;
}

/* ================================================================
 * 文件阅读器
 *
 * 打开 sd_path，分页显示内容：
 *   KEY2 = 下一页（局刷）
 *   KEY1 = 上一页（局刷）
 *   KEY4 = 关闭文件，返回调用者
 *
 * 首页使用全刷（消除文件浏览器残影）。
 * 后续翻页使用局刷（速度更快）。
 * 第 9 行显示状态栏（当前页码 + 按键提示）。
 * ================================================================ */
static void app_show_file(const char *sd_path)
{
    DBG_Print("\r\n================================\r\n");
    DBG_Fmt("  app_show_file: %s\r\n", sd_path);
    DBG_Print("================================\r\n\r\n");

    DBG_Print("[1] Mount SD...\r\n");
    SD_LOCK();
    SD_Err serr = SD_Mount();
    if (serr != SD_OK) {
        DBG_Fmt("    FAIL code=%d\r\n", (int)serr);
        SD_UNLOCK();
        return;
    }
    DBG_Print("    OK\r\n");

    FIL fp;
    DBG_Fmt("[2] Open: %s\r\n", sd_path);
    if (f_open(&fp, sd_path, FA_READ) != FR_OK) {
        DBG_Print("    f_open FAIL\r\n");
        SD_Unmount();
        SD_UNLOCK();
        return;
    }
    DBG_Fmt("    %lu bytes\r\n\r\n", (unsigned long)f_size(&fp));
    BM_SetLast(sd_path);    /* 主页"继续阅读"依赖此记录 */

    DBG_Print("[3] EPD init...\r\n");
    epd_wake();
    DBG_Print("    OK\r\n\r\n");

    /* 查询书签，从头扫描重建书签前的页偏移表（使 KEY1 向前翻页可用）*/
    uint32_t resume_off = BM_Load(sd_path);
    s_page_offsets[0]   = 0u;
    s_page_count        = 1u;
    uint8_t  cur_page   = 0u;
    uint32_t next_off;

    if (resume_off > 0u) {
        DBG_Fmt("[BM] Resume off=%lu, scanning...\r\n", (unsigned long)resume_off);
        s_scan_mode = 1u;
        uint32_t scan_off = 0u;
        for (;;) {
            uint32_t nxt = render_page_from_file(&fp, scan_off);
            if (nxt == FILE_EOF || nxt > resume_off) break;
            if (s_page_count < MAX_PAGES) {
                s_page_offsets[s_page_count++] = nxt;
            } else {
                /* 滑动窗口：丢弃最旧的页，保留最近 MAX_PAGES 条 */
                memmove(&s_page_offsets[0], &s_page_offsets[1],
                        (MAX_PAGES - 1u) * sizeof(uint32_t));
                s_page_offsets[MAX_PAGES - 1u] = nxt;
            }
            scan_off = nxt;
            if (nxt == resume_off) break;
        }
        s_scan_mode = 0u;
        cur_page = s_page_count - 1u;
        DBG_Fmt("[BM] Resumed at Pg.%d\r\n", (int)(cur_page + 1u));
    }

    /* 渲染起始页 + 状态栏，全刷显示 */
    DBG_Fmt("[4] Page %d:\r\n----\r\n", (int)(cur_page + 1u));
    next_off = render_page_from_file(&fp, s_page_offsets[cur_page]);
    {
        char st[48];
        if (next_off == FILE_EOF) {
            snprintf(st, sizeof(st),
                     (cur_page > 0u) ? "Pg.%d[END] K1:prev K4:back" : "Pg.%d[END] K4:back",
                     (int)(cur_page + 1u));
        } else {
            snprintf(st, sizeof(st),
                     (cur_page > 0u) ? "Pg.%d K1:prev K2:next K4:back" : "Pg.%d K2:next K4:back",
                     (int)(cur_page + 1u));
        }
        GUI_ShowText(0u, (uint16_t)(PAGE_LINES * LINE_H), (uint8_t *)st, FONT_16X16, GUI_BLACK);
        DBG_Fmt("----\r\n%s\r\n\r\n", st);
    }
    epd_cmd(EPD_CMD_DISP_FULL, s_frame);   /* 全刷：消除浏览器残影 */

    /* 按键事件循环（阻塞在队列上，不占 CPU） */
    for (;;) {
        KeyMsg_t msg;
        osMessageQueueGet(g_key_queue, &msg, 0u, osWaitForever);
        if (msg.evt != KEY_EVT_SHORT) continue;

        switch (msg.id) {

        case KEY4:  /* 退出阅读器 */
            goto file_exit;

        case KEY2:  /* 下一页 */
            if (next_off == FILE_EOF) {
                DBG_Print("(last page)\r\n");
            } else {
                cur_page++;
                if (cur_page >= s_page_count && s_page_count < MAX_PAGES) {
                    s_page_offsets[s_page_count++] = next_off;
                }
                DBG_Fmt("[4] Page %d:\r\n----\r\n", (int)(cur_page + 1u));
                next_off = render_page_from_file(&fp, s_page_offsets[cur_page]);
                char st[48];
                snprintf(st, sizeof(st),
                         (next_off == FILE_EOF) ? "Pg.%d[END] K1:prev K4:back"
                                                : "Pg.%d K1:prev K2:next K4:back",
                         (int)(cur_page + 1u));
                GUI_ShowText(0u, (uint16_t)(PAGE_LINES * LINE_H), (uint8_t *)st, FONT_16X16, GUI_BLACK);
                DBG_Fmt("----\r\n%s\r\n\r\n", st);
                epd_cmd(EPD_CMD_DISP_PART, s_frame);
            }
            break;

        case KEY1:  /* 上一页 */
            if (cur_page == 0u) {
                DBG_Print("(first page)\r\n");
            } else {
                cur_page--;
                DBG_Fmt("[4] Page %d:\r\n----\r\n", (int)(cur_page + 1u));
                next_off = render_page_from_file(&fp, s_page_offsets[cur_page]);
                char st[48];
                snprintf(st, sizeof(st),
                         (next_off == FILE_EOF) ? "Pg.%d[END] K1:prev K4:back"
                                                : "Pg.%d K1:prev K2:next K4:back",
                         (int)(cur_page + 1u));
                GUI_ShowText(0u, (uint16_t)(PAGE_LINES * LINE_H), (uint8_t *)st, FONT_16X16, GUI_BLACK);
                DBG_Fmt("----\r\n%s\r\n\r\n", st);
                epd_cmd(EPD_CMD_DISP_PART, s_frame);
            }
            break;

        default:
            break;
        }
    }

file_exit:
    /* 保存书签：记录当前页的文件字节偏移，下次打开时恢复 */
    BM_Save(sd_path, s_page_offsets[cur_page]);
    DBG_Fmt("[BM] Saved offset=%lu\r\n", (unsigned long)s_page_offsets[cur_page]);

    f_close(&fp);
    SD_Unmount();
    SD_UNLOCK();   /* 释放 SD 互斥量，wifi_task 可以再次访问 SD */
    epd_cmd(EPD_CMD_SLEEP, NULL);

    DBG_Print("\r\n================================\r\n");
    DBG_Print("  File closed. Back to browser.\r\n");
    DBG_Print("================================\r\n");
}

/* ================================================================
 * 文件浏览器
 *
 * 扫描 SD 卡 dir_path 目录，将文件列表显示在 EPD：
 *   KEY2 = 光标下移（循环）  —— 局刷
 *   KEY1 = 光标上移（循环）  —— 局刷
 *   KEY3 = 打开选中文件      —— 进入 app_show_file
 *
 * 文件名为 GBK 编码，在 EPD 上显示正确；UTF-8 串口终端会乱码。
 * ================================================================ */
static void app_file_browser(const char *dir_path)
{
    DBG_Print("\r\n================================\r\n");
    DBG_Fmt("  app_file_browser: %s\r\n", dir_path);
    DBG_Print("================================\r\n\r\n");

    /* 扫描 SD 卡文件列表 */
    DBG_Print("[1] Mount SD...\r\n");
    SD_LOCK();
    SD_Err err = SD_Mount();
    if (err != SD_OK) {
        DBG_Fmt("    FAIL code=%d\r\n", (int)err);
        SD_UNLOCK();
        return;
    }
    DBG_Print("    OK\r\n\r\n");

    s_file_count = 0u;
    DIR     dir;
    FILINFO fno;

    if (f_opendir(&dir, dir_path) == FR_OK) {
        while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != '\0') {
            if (fno.fattrib & AM_DIR) continue;
            if (s_file_count >= MAX_FILES) break;
            strncpy(s_filenames[s_file_count], fno.fname, MAX_NAME - 1u);
            s_filenames[s_file_count][MAX_NAME - 1u] = '\0';
            DBG_Fmt("[%d] %s\r\n", (int)(s_file_count + 1u), s_filenames[s_file_count]);
            s_file_count++;
        }
        f_closedir(&dir);
    } else {
        DBG_Print("    open dir FAIL\r\n");
    }

    SD_Unmount();
    SD_UNLOCK();
    DBG_Fmt("Total: %d files\r\n\r\n", (int)s_file_count);

    /* 按文件名字母顺序排序（FAT 目录项顺序不等于下载顺序，删除后会复用旧槽位）*/
    for (uint8_t i = 1u; i < s_file_count; i++) {
        char tmp[MAX_NAME];
        uint8_t j = i;
        memcpy(tmp, s_filenames[i], MAX_NAME);
        while (j > 0u && strcmp(s_filenames[j - 1u], tmp) > 0) {
            memcpy(s_filenames[j], s_filenames[j - 1u], MAX_NAME);
            j--;
        }
        memcpy(s_filenames[j], tmp, MAX_NAME);
    }

    /* 清除书签表中已不存在的孤立条目（SD 文件被删除后自动回收槽位）*/
    {
        char sweep_pfx[32u];
        snprintf(sweep_pfx, sizeof(sweep_pfx), "%s/", dir_path);
        BM_Sweep(sweep_pfx, (const char *)s_filenames, MAX_NAME, s_file_count);
    }

    if (s_file_count == 0u) {
        DBG_Print("No files. Exit.\r\n");
        return;
    }

    /* 初始化 EPD，全刷显示文件列表 */
    DBG_Print("[2] EPD init...\r\n");
    epd_wake();
    DBG_Print("    OK\r\n\r\n");

    uint8_t selected   = 0u;
    uint8_t view_start = 0u;
    draw_list_full(selected, view_start);
    DBG_Print("KEY2=down  KEY1=up  KEY3=open\r\n");
    DBG_Fmt("Cursor: [%d] %s\r\n", (int)(selected + 1u), s_filenames[selected]);

    /* 按键事件循环 */
    for (;;) {
        KeyMsg_t msg;
        osMessageQueueGet(g_key_queue, &msg, 0u, osWaitForever);

        /* K4 长按：返回主页 */
        if (msg.id == KEY4 && msg.evt == KEY_EVT_LONG) {
            DBG_Print("[Browser] K4 long: back to home\r\n");
            epd_cmd(EPD_CMD_SLEEP, NULL);
            return;
        }

        if (msg.evt != KEY_EVT_SHORT) continue;    /* 其余按键只响应单击 */

        switch (msg.id) {

        case KEY2:  /* 光标下移，到底则回到首个 */
            if (selected < s_file_count - 1u) {
                selected++;
                if (selected >= view_start + MAX_VISIBLE) view_start++;
            } else {
                selected   = 0u;
                view_start = 0u;
            }
            DBG_Fmt("Cursor -> [%d] %s\r\n", (int)(selected + 1u), s_filenames[selected]);
            draw_list_part(selected, view_start);
            break;

        case KEY1:  /* 光标上移，到顶则回到最后一个 */
            if (selected > 0u) {
                selected--;
                if (selected < view_start) view_start--;
            } else {
                selected   = s_file_count - 1u;
                view_start = (s_file_count > MAX_VISIBLE) ? s_file_count - MAX_VISIBLE : 0u;
            }
            DBG_Fmt("Cursor -> [%d] %s\r\n", (int)(selected + 1u), s_filenames[selected]);
            draw_list_part(selected, view_start);
            break;

        case KEY3:  /* 打开选中文件 */
            {
                DBG_Fmt("[Open] %s\r\n", s_filenames[selected]);
                char path[MAX_NAME + 4u];
                snprintf(path, sizeof(path), "%s/%s", dir_path, s_filenames[selected]);

                app_show_file(path);   /* 阅读器内部处理 KEY1/KEY2/KEY4，KEY4 按下后返回 */

                /* 阅读器返回：重新初始化 EPD，全刷显示文件列表 */
                DBG_Print("Back to browser\r\n");
                epd_wake();
                draw_list_full(selected, view_start);
                DBG_Print("KEY2=down  KEY1=up  KEY3=open\r\n");
                DBG_Fmt("Cursor: [%d] %s\r\n", (int)(selected + 1u), s_filenames[selected]);
            }
            break;

        default:
            break;
        }
    }
}

/* ================================================================
 * 主页屏幕
 *
 * 启动后立刻全刷加载画面，随后挂载 SD 获取最近阅读信息，
 * 再次全刷显示主界面。
 *   KEY2 = 继续阅读上次文件（有记录时）
 *   KEY1 / KEY4 = 进入书架（文件浏览器）
 * ================================================================ */
static void app_home_screen(const char *dir_path)
{
    DBG_Print("\r\n================================\r\n");
    DBG_Print("  app_home_screen\r\n");
    DBG_Print("================================\r\n\r\n");

    /* 立即显示加载画面，不等 SD */
    epd_wake();
    GUI_ShowText(92u,  32u, (uint8_t *)"Sprite's Lover", FONT_16X16, GUI_BLACK);
    GUI_ShowText(100u, 64u, (uint8_t *)"E-Ink Reader",   FONT_16X16, GUI_BLACK);
    GUI_ShowText(108u, 96u, (uint8_t *)"Loading...",     FONT_16X16, GUI_BLACK);
    epd_cmd(EPD_CMD_DISP_FULL, s_frame);

    /* 从书签 RAM 缓存读取最近文件路径（BM_Init 已在 App_Task 预加载）*/
    char     last_path[60u] = {0};
    uint32_t last_off       = 0u;
    uint32_t file_size      = 0u;
    uint8_t  has_last       = 0u;
    uint8_t  sd_ok          = 0u;

    BM_GetLast(last_path, (uint8_t)sizeof(last_path));
    if (last_path[0] != '\0') {
        last_off = BM_Load(last_path);
        has_last = 1u;
        DBG_Fmt("[HOME] Last: %s\r\n", last_path);
        DBG_Fmt("[HOME] Saved off=%lu\r\n", (unsigned long)last_off);
    } else {
        DBG_Print("[HOME] No last file\r\n");
    }

    /* 挂载 SD：获取文件大小以计算进度，同时验证文件仍存在 */
    DBG_Print("[HOME] Mount SD...\r\n");
    if (osMutexAcquire(g_sd_mutex, 3000u) == osOK) {
        if (SD_Mount() == SD_OK) {
            sd_ok = 1u;
            DBG_Print("    OK\r\n");
            if (has_last) {
                FIL fp;
                if (f_open(&fp, last_path, FA_READ) == FR_OK) {
                    file_size = (uint32_t)f_size(&fp);
                    f_close(&fp);
                    DBG_Fmt("    file size=%lu B\r\n", (unsigned long)file_size);
                } else {
                    DBG_Print("    f_open FAIL (file deleted?)\r\n");
                    has_last = 0u;
                }
            }
            SD_Unmount();
        } else {
            DBG_Print("    FAIL\r\n");
        }
        osMutexRelease(g_sd_mutex);
    } else {
        DBG_Print("[HOME] SD busy, skip size check\r\n");
    }

    /* 渲染主界面 */
    Canvas_Clear(GUI_WHITE);
    GUI_ShowText(92u,  0u,  (uint8_t *)"Sprite's Lover", FONT_16X16, GUI_BLACK);
    GUI_ShowText(100u, 16u, (uint8_t *)"E-Ink Reader",   FONT_16X16, GUI_BLACK);

    char line[80u];
    if (has_last) {
        /* 提取文件名：去路径前缀，去扩展名 */
        const char *fname = last_path;
        const char *sl    = strrchr(last_path, '/');
        if (sl != NULL) fname = sl + 1;

        char name_buf[60u];
        strncpy(name_buf, fname, sizeof(name_buf) - 1u);
        name_buf[sizeof(name_buf) - 1u] = '\0';
        char *dot = strrchr(name_buf, '.');
        if (dot != NULL) *dot = '\0';

        /* 进度：乘以 100000 再整除，得到带三位小数的百分比
         * 例：offset=1549, size=735926 → 210 → "0.210%" */
        uint32_t pct1000 = (file_size > 0u)
                         ? (uint32_t)((uint64_t)last_off * 100000u / (uint64_t)file_size)
                         : 0u;
        DBG_Fmt("[HOME] Progress: %lu.%03lu%%\r\n",
                (unsigned long)(pct1000 / 1000u),
                (unsigned long)(pct1000 % 1000u));

        GUI_ShowText(0u, 48u, (uint8_t *)"Last:",   FONT_16X16, GUI_BLACK);
        GUI_ShowText(0u, 64u, (uint8_t *)name_buf,  FONT_16X16, GUI_BLACK);
        snprintf(line, sizeof(line), "Progress: %lu.%03lu%%",
                 (unsigned long)(pct1000 / 1000u),
                 (unsigned long)(pct1000 % 1000u));
        GUI_ShowText(0u, 80u, (uint8_t *)line, FONT_16X16, GUI_BLACK);
    } else {
        GUI_ShowText(0u, 48u, (uint8_t *)"Welcome!",    FONT_16X16, GUI_BLACK);
        GUI_ShowText(0u, 64u, (uint8_t *)"No history.", FONT_16X16, GUI_BLACK);
    }

    /* 状态信息（倒数第二行 y=112）*/
    snprintf(line, sizeof(line), "SD:%s  FATFS:OK  GBK", sd_ok ? "OK" : "ERR");
    GUI_ShowText(0u, 112u, (uint8_t *)line, FONT_16X16, GUI_BLACK);

    /* 按键提示（最后一行 y=128）*/
    if (has_last) {
        GUI_ShowText(0u, (uint16_t)(PAGE_LINES * LINE_H),
                     (uint8_t *)"K1/K2:List  K3:Open", FONT_16X16, GUI_BLACK);
    } else {
        GUI_ShowText(0u, (uint16_t)(PAGE_LINES * LINE_H),
                     (uint8_t *)"K1/K2:List", FONT_16X16, GUI_BLACK);
    }
    epd_cmd(EPD_CMD_DISP_FULL, s_frame);

    DBG_Print("[HOME] Ready. Waiting key...\r\n\r\n");

    /* 按键事件循环 */
    for (;;) {
        KeyMsg_t msg;
        osMessageQueueGet(g_key_queue, &msg, 0u, osWaitForever);
        if (msg.evt != KEY_EVT_SHORT) continue;

        switch (msg.id) {
        case KEY1:
        case KEY2:
            DBG_Print("[HOME] K1/K2: to browser\r\n");
            epd_cmd(EPD_CMD_SLEEP, NULL);
            app_file_browser(dir_path);
            return;
        case KEY3:
            if (has_last) {
                DBG_Print("[HOME] K3: open last file\r\n");
                epd_cmd(EPD_CMD_SLEEP, NULL);
                app_show_file(last_path);
                /* 阅读器 K4 退出后进文件列表，而非回主页 */
                app_file_browser(dir_path);
                return;
            }
            break;
        default:
            break;
        }
    }
}

/* ================================================================
 * App_Task — 由 main.c 的 StartDefaultTask 调用
 * ================================================================ */
void App_Task(void *arg)
{
    (void)arg;
    BM_Init();          /* 从 Flash 预加载书签表及最近文件路径到 RAM */
    for (;;) {
        app_home_screen("0:/ebooks");
    }
}
