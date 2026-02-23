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


FATFS fs;							/* FatFs文件系统对象 */
FIL fnew;							/* 文件对象 */
FRESULT res_sd;						/* 文件操作结果 */
UINT fnum;							/* 文件成功读写数量 */
BYTE ReadBuffer[256]="还未写入";		/* 读缓冲区 */



u8 ImageBW[5624];

int main(void)
{

	
	board_init();
	uart1_init(115200);
	sys_Timer_Init();
	bsp_key_init();
	bsp_spi_flash_init();		
	printf("2-1 FatFs_SD读取文件内容显示\r\n");
	


	
	res_sd = f_mount(&fs,"0:",1);
	printf("res_sd = %d\r\n",res_sd);
	printf("KEYUP   : 读出文件测试\r\n");
	printf("KEYLEFT : 显示读出文件测试\r\n");

	EPD_GPIOInit();
	EPD_Init();
	Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);			// 绑定画布
	Paint_Clear(WHITE);

	// 上电先全屏清白并全刷一次，去残影
	EPD_Display_Clear();
	EPD_Update();
	EPD_Clear_R26H();											// 进入局刷对比模式
	delay_ms(1000);
	
	printf("success\r\n");


	while(1)
	{
		KEYS waitKey = Key_GetNum();		
		if(waitKey == KEY_UP)
		{
/*------------------- 文件系统测试：读测试 ------------------------------------*/
			printf("****** 即将进行文件读取测试... ******\r\n");
			res_sd = f_open(&fnew, "0:《水浒传》1.txt", FA_OPEN_EXISTING | FA_READ); 	 
			if(res_sd == FR_OK)
			{
				
				printf("》打开文件成功。\r\n");
				res_sd = f_read(&fnew, ReadBuffer, sizeof(ReadBuffer), &fnum); 
			if(res_sd==FR_OK)
			{
			  printf("》文件读取成功,读到字节数据：%d\r\n",fnum);
			  printf("》读取得的文件数据为：\r\n%s \r\n", ReadBuffer);	
			  


				
				
			}
			else
			{
			  printf("！！文件读取失败：(%d)\n",res_sd);
			}		
			}
			else
			{
				
				printf("！！打开文件失败。\r\n");
			}
			/* 不再读写，关闭文件 */
			f_close(&fnew);	
		
		}
	
		if(waitKey == KEY_RIGHT)
		{	
			Paint_Clear(WHITE);

			EPD_ShowMixedString(0,0,ReadBuffer,FONT_24X24,24,BLACK);
			
			
			
			EPD_Display(ImageBW);
			EPD_PartUpdate();
	
		}			
	}
}

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{

		Ket_Tick();
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	}
}
