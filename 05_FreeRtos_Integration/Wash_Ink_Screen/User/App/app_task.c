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
    SD_Err serr = SD_Mount();
    if (serr != SD_OK) {
        DBG_Fmt("    FAIL code=%d\r\n", (int)serr);
        return;
    }
    DBG_Print("    OK\r\n");

    FIL fp;
    DBG_Fmt("[2] Open: %s\r\n", sd_path);
    if (f_open(&fp, sd_path, FA_READ) != FR_OK) {
        DBG_Print("    f_open FAIL\r\n");
        SD_Unmount();
        return;
    }
    DBG_Fmt("    %lu bytes\r\n\r\n", (unsigned long)f_size(&fp));

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
    SD_Err err = SD_Mount();
    if (err != SD_OK) {
        DBG_Fmt("    FAIL code=%d\r\n", (int)err);
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
    DBG_Fmt("Total: %d files\r\n\r\n", (int)s_file_count);

    /* 清除书签表中已不存在的孤立条目（SD 文件被删除后自动回收槽位）*/
    BM_Sweep("0:/", (const char *)s_filenames, MAX_NAME, s_file_count);

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
        if (msg.evt != KEY_EVT_SHORT) continue;

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
                snprintf(path, sizeof(path), "0:/%s", s_filenames[selected]);

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
 * App_Task — 由 main.c 的 StartDefaultTask 调用
 * ================================================================ */
void App_Task(void *arg)
{
    (void)arg;
    BM_Init();              /* 从 Flash 预加载书签表到 RAM */
    app_file_browser("0:/");
    /* app_file_browser 正常情况下不返回；若返回则在此挂起 */
    for (;;) {
        osDelay(1000u);
    }
}
