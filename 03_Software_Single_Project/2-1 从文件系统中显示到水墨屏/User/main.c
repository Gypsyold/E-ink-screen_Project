#include "stm32f4xx.h"                  // Device header
#include "board.h"
#include "bsp_uart.h"
#include "stdio.h"
#include "bsp_key.h"
#include "sys_timer.h"
#include "ff.h"
#include "spi_w25Q128_flash.h"
#include "EPD_GUI.h"
#include <string.h>

// ====================== 【配置区】可根据实际情况修改 ======================
#define MAX_SHOW_BYTES    144       // 一屏最大处理/显示字节数（贴合屏幕）
#define READ_BUF_SIZE     144       // 读取缓冲区大小（和显示一致，精准控制）
#define NOVEL_FILE_PATH   "0:《水浒传》.txt" // 小说文件路径
#define PAGE_HISTORY_MAX  20        // 最大历史页数（可调整）
#define SCREEN_WIDTH      296       // 屏幕宽度（px）
#define SCREEN_HEIGHT     144       // 屏幕高度（px）

// ====================== 【GBK编码区】100%准确的GBK解析 ======================
// 功能：从buf的start位置开始，解析一个完整的GBK/ASCII字符
// 返回值：解析完这个字符后的下一个位置（即下一个字符的起始位置）
// 如果start位置是截断的GBK首字节（没有尾字节），返回start（表示没解析完）
u16 get_next_char_boundary(u8 *buf, u16 start, u16 max_len)
{
    if(start >= max_len) return start;
    
    u8 byte1 = buf[start];
    
    // 情况1：ASCII字符（0x00-0x7F）
    if(byte1 <= 0x7F)
    {
        return start + 1; // 占1字节
    }
    
    // 情况2：GBK首字节（0x81-0xFE）
    if(byte1 >= 0x81 && byte1 <= 0xFE)
    {
        // 检查是否有尾字节
        if(start + 1 >= max_len)
        {
            return start; // 没有尾字节，是截断的
        }
        
        u8 byte2 = buf[start + 1];
        // 检查尾字节是否合法（0x40-0xFE且≠0x7F）
        if(byte2 >= 0x40 && byte2 <= 0xFE && byte2 != 0x7F)
        {
            return start + 2; // 合法GBK汉字，占2字节
        }
    }
    
    // 情况3：无效字符，跳过1字节
    return start + 1;
}

// 功能：判断buf的前len字节中，最后一个字符是不是完整的
// 返回值：1=最后一个字符是截断的GBK首字节；0=最后一个字符是完整的
// 同时通过 *last_valid_end 返回最后一个完整字符的结束位置
u8 is_truncated_and_get_boundary(u8 *buf, u16 len, u16 *last_valid_end)
{
    u16 curr_pos = 0;
    u16 prev_pos = 0;
    
    // 从开头逐字节解析，直到len
    while(curr_pos < len)
    {
        prev_pos = curr_pos;
        curr_pos = get_next_char_boundary(buf, curr_pos, len);
        
        // 如果curr_pos没有前进，说明是截断的GBK首字节
        if(curr_pos == prev_pos)
        {
            *last_valid_end = prev_pos; // 最后一个完整字符在prev_pos结束
            printf("》【精准解析】检测到截断的GBK首字节：0x%02X，位置：%d\r\n", buf[curr_pos], curr_pos);
            return 1;
        }
    }
    
    // 解析完了len字节，且最后一个字符是完整的
    *last_valid_end = len;
    printf("》【精准解析】最后一个字符是完整的，结束位置：%d\r\n", len);
    return 0;
}

// ====================== 【全局变量区】 ======================
FATFS fs;							/* FatFs文件系统对象 */
FIL fnew;							/* 文件对象 */
FRESULT res_sd;						/* 文件操作结果 */
UINT fnum;							/* 文件成功读写数量 */
BYTE ReadBuffer[READ_BUF_SIZE] = "还未写入";	/* 读缓冲区 */

u8 leftover_byte = 0;        // 被截断的GBK首字节（0=无遗留）
u8 has_leftover = 0;         // 是否有遗留字节标记
u32 file_offset = 0;         // 当前文件偏移量（页起始位置）
u32 page_history[PAGE_HISTORY_MAX];  // page_history[i] = 第i+1页的起始偏移
u8 current_page = 0;         // 当前显示的页数（从0开始，0=第1页）
u8 max_saved_page = 0;       // 已保存的最大页数（避免越界）
u8 is_file_open = 0;         // 文件是否已打开标记
u8 is_file_end = 0;          // 文件是否读完标记

u8 ImageBW[5624];

// ====================== 【翻页逻辑区】核心翻页显示函数 ======================
// 新增：专门用于获取下一页起始偏移的函数（不显示，只计算）
u32 GetNextPageOffset(u32 curr_offset)
{
    u32 temp_offset = curr_offset;
    u8 temp_leftover = leftover_byte;
    u8 temp_has_leftover = has_leftover;
    UINT temp_read_len = READ_BUF_SIZE;
    BYTE temp_buf[READ_BUF_SIZE] = {0};
    UINT temp_fnum = 0;
    
    // 模拟处理遗留字节
    if(temp_has_leftover)
    {
        temp_buf[0] = temp_leftover;
        temp_read_len = READ_BUF_SIZE - 1;
        temp_leftover = 0;
        temp_has_leftover = 0;
    }
    
    // 移动文件指针
    res_sd = f_lseek(&fnew, temp_offset);
    if(res_sd != FR_OK)
    {
        printf("！！获取下一页偏移失败，错误码：%d\r\n",res_sd);
        return curr_offset;
    }
    
    // 读取数据
    res_sd = f_read(&fnew, &temp_buf[temp_has_leftover ? 1 : 0], temp_read_len, &temp_fnum);
    if(res_sd != FR_OK)
    {
        printf("！！读取数据失败，错误码：%d\r\n",res_sd);
        return curr_offset;
    }
    
    // 计算显示字节数
    uint16_t actual_used = EPD_ShowMixedString(0,0,temp_buf,FONT_24X24,24,BLACK);
    u16 last_valid_end = 0;
    u32 skip_bytes = actual_used;
    
    // 判断截断
    if(actual_used == MAX_SHOW_BYTES)
    {
        if(is_truncated_and_get_boundary(temp_buf, actual_used, &last_valid_end))
        {
            temp_leftover = temp_buf[last_valid_end];
            temp_has_leftover = 1;
            skip_bytes = last_valid_end;
        }
        else
        {
            skip_bytes = last_valid_end;
        }
    }
    
    // 恢复原始状态
    leftover_byte = temp_leftover;
    has_leftover = temp_has_leftover;
    
    // 返回下一页起始偏移
    return curr_offset + skip_bytes;
}

void Novel_ShowOnePage(u8 read_only)
{
	// 保存原始偏移量（上翻时恢复）
	u32 orig_offset = file_offset;
	UINT read_len = READ_BUF_SIZE;

	// 1. 清空整个读取缓冲区
	memset(ReadBuffer, 0, READ_BUF_SIZE);

	// 2. 处理遗留字节（100%准确的判断逻辑）
	if(has_leftover)
	{
		ReadBuffer[0] = leftover_byte;
		read_len = READ_BUF_SIZE - 1;
		printf("》有遗留字节：0x%02X，读取%d字节补全缓冲区\r\n", leftover_byte, read_len);
		leftover_byte = 0;
		has_leftover = 0;
	}

	// 3. 移动文件指针到当前偏移量
	res_sd = f_lseek(&fnew, file_offset);
	if(res_sd != FR_OK)
	{
		printf("！！移动文件指针失败，错误码：%d\r\n",res_sd);
		return;
	}

	// 4. 读取数据
	res_sd = f_read(&fnew, &ReadBuffer[has_leftover ? 1 : 0], read_len, &fnum);
	if(res_sd != FR_OK)
	{
		printf("！！读取文件失败，错误码：%d\r\n",res_sd);
		return;
	}
	
	// 检查是否到文件末尾
	if(fnum < read_len)
	{
		printf("》文件剩余字节：%d（已到末尾）\r\n",fnum);
		is_file_end = 1;
	}

	// 5. 清屏并显示内容
	Paint_Clear(WHITE);
	uint16_t actual_used = EPD_ShowMixedString(0,0,ReadBuffer,FONT_24X24,24,BLACK);
	printf("》本屏显示了 %d 个字节\r\n",actual_used);
	
	// 6. 刷新屏幕
	EPD_Display(ImageBW);
	EPD_PartUpdate();

	// 7. 下翻模式：使用精准解析判断截断（仅更新偏移，不保存历史）
	if(read_only == 0) 
	{
		u16 last_valid_end = 0;
		u32 skip_bytes = actual_used;
		
		// 仅当满屏显示时，才判断截断
		if(actual_used == MAX_SHOW_BYTES)
		{
			// 核心：调用100%准确的精准解析函数
			if(is_truncated_and_get_boundary(ReadBuffer, actual_used, &last_valid_end))
			{
				// 最后一个字符是截断的GBK首字节
				leftover_byte = ReadBuffer[last_valid_end];
				has_leftover = 1;
				skip_bytes = last_valid_end; // 只跳转到最后一个完整字符的结束位置
				printf("》检测到截断的GBK首字节：0x%02X，偏移量+%d\r\n", leftover_byte, skip_bytes);
			}
			else
			{
				// 最后一个字符是完整的
				skip_bytes = last_valid_end;
				printf("》最后一个字符是完整的，偏移量+%d\r\n", skip_bytes);
			}
		}
		else
		{
			// 没满屏，直接用actual_used
			skip_bytes = actual_used;
		}
		
		// 更新偏移量（仅为下一页做准备）
		file_offset += skip_bytes;
		printf("》下一页起始偏移：%lu\r\n",file_offset);
	}
	else // 上翻模式
	{
		// 上翻页时强制恢复原始偏移，清空所有状态
		file_offset = orig_offset;
		leftover_byte = 0;
		has_leftover = 0;
		is_file_end = 0;
		printf("》上翻页模式，偏移量保持为页起始：%lu\r\n",file_offset);
	}
}

// ====================== 【主函数区】 ======================
int main(void)
{
	board_init();
	uart1_init(115200);
	sys_Timer_Init();
	bsp_key_init();
	bsp_spi_flash_init();		
	printf("2-1 最终完善版：100%准确GBK解析+无乱码翻页\r\n");

	res_sd = f_mount(&fs,"0:",1);
	printf("res_sd = %d\r\n",res_sd);
	// 适配新的按键映射提示
	printf("KEYUP   : 打开小说文件并显示第一页\r\n");
	printf("KEYDOWN : 保留原功能（测试显示）\r\n");
	printf("KEYRIGHT: 翻到下一页\r\n");
	printf("KEYLEFT : 翻到上一页\r\n");

	EPD_GPIOInit();
	EPD_Init();
	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// 绑定画布
	Paint_Clear(WHITE);

	// 上电清屏去残影
	EPD_Display_Clear();
	EPD_Update();
	EPD_Clear_R26H();											
	delay_ms(1000);
	
	printf("success\r\n");

	// 初始化变量
	memset(page_history, 0, sizeof(page_history));
	leftover_byte = 0;
	has_leftover = 0;
	current_page = 0;
	max_saved_page = 0;
	page_history[0] = 0; // 预存第一页偏移

	while(1)
	{
		KEYS waitKey = Key_GetNum();		
		if(waitKey == KEY_UP)
		{
/*------------------- 打开文件并显示第一页 ------------------------------*/
			printf("****** 打开小说文件并显示第一页... ******\r\n");
			// 先关闭已打开的文件
			if(is_file_open)
			{
				f_close(&fnew);
				is_file_open = 0;
			}
			
			// 重置所有变量（核心：重置页数相关）
			leftover_byte = 0;
			has_leftover = 0;
			file_offset = 0;
			is_file_end = 0;
			current_page = 0;          // 回到第1页
			max_saved_page = 0;        // 已保存页数重置
			memset(page_history, 0, sizeof(page_history));
			page_history[0] = 0;       // 第1页偏移固定为0
			
			// 打开小说文件
			res_sd = f_open(&fnew, NOVEL_FILE_PATH, FA_OPEN_EXISTING | FA_READ); 	 
			if(res_sd == FR_OK)
			{
				printf("》打开小说文件成功。\r\n");
				is_file_open = 1;       
				// 显示第1页（下翻模式）
				Novel_ShowOnePage(0);
				// 保存第2页的起始偏移（提前计算）
				if(max_saved_page < 1)
				{
					page_history[1] = file_offset;
					max_saved_page = 1;
				}
			}
			else
			{
				printf("！！打开小说文件失败，错误码：%d\r\n",res_sd);
			}
		}

		// KEY_DOWN：测试显示功能（你的新映射）
		if(waitKey == KEY_DOWN)
		{	
			Paint_Clear(WHITE);
			uint16_t actual_used_len = EPD_ShowMixedString(0,0,ReadBuffer,FONT_24X24,24,BLACK);
			printf("测试显示了 %d 个字节\r\n",actual_used_len);
			EPD_Display(ImageBW);
			EPD_PartUpdate();
		}		
		
		// KEY_RIGHT：翻到下一页（核心修复）
		if(waitKey == KEY_RIGHT)
		{
			if(!is_file_open)
			{
				printf("！！请先按KEY_UP打开小说文件\r\n");
				continue;
			}
			if(is_file_end)
			{
				printf("！！已到小说末尾，无法翻下一页\r\n");
				continue;
			}
			
			// 核心：下翻页=页数+1
			u8 next_page = current_page + 1;
			if(next_page >= PAGE_HISTORY_MAX)
			{
				printf("！！历史页数已达上限（%d页）\r\n", PAGE_HISTORY_MAX);
				continue;
			}
			
			// 步骤1：设置当前页的起始偏移（从历史数组读取）
			file_offset = page_history[next_page];
			leftover_byte = 0;
			has_leftover = 0;
			is_file_end = 0;
			
			// 步骤2：显示当前页
			Novel_ShowOnePage(0);
			
			// 步骤3：提前计算并保存下一页的起始偏移
			if(next_page + 1 <= PAGE_HISTORY_MAX - 1)
			{
				page_history[next_page + 1] = file_offset;
				if(next_page + 1 > max_saved_page)
				{
					max_saved_page = next_page + 1;
				}
			}
			
			// 步骤4：更新当前页数
			current_page = next_page;
			printf("》翻到第%d页，当前页起始偏移：%lu，下一页起始偏移：%lu\r\n", 
			       current_page+1, page_history[current_page], page_history[current_page+1]);
		}

		// KEY_LEFT：翻到上一页（核心修复）
		if(waitKey == KEY_LEFT)
		{
			if(!is_file_open)
			{
				printf("！！请先按KEY_UP打开小说文件\r\n");
				continue;
			}
			// 核心：上翻页=页数-1，不能小于0（第1页）
			if(current_page == 0) 
			{
				printf("！！已到小说开头，无法翻上一页\r\n");
				continue;
			}
			
			// 步骤1：回退页数
			current_page--;
			
			// 步骤2：设置当前页的起始偏移（从历史数组读取）
			file_offset = page_history[current_page];
			leftover_byte = 0;
			has_leftover = 0;
			is_file_end = 0;
			
			// 步骤3：显示当前页
			Novel_ShowOnePage(1);
			
			printf("》翻到第%d页，起始偏移：%lu\r\n", current_page+1, file_offset);
		}
	}
}

// ====================== 【中断服务函数区】 ======================
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		Ket_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}

