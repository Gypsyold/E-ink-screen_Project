#include "stm32f4xx.h"                  // Device header
#include "board.h"
#include "bsp_uart.h"
#include "stdio.h"
#include "bsp_key.h"
#include "sys_timer.h"
#include "ff.h"



FATFS fs;							/* FatFs文件系统对象 */
FIL fnew;							/* 文件对象 */
FRESULT res_sd;						/* 文件操作结果 */
UINT fnum;							/* 文件成功读写数量 */
BYTE ReadBuffer[1024]={0};			/* 读缓冲区 */
BYTE WriteBuffer[] =				/* 写缓冲区*/
"标准库的SD卡的FatFs文件系统构建成功！！！\r\n";  



int main(void)
{

	
	board_init();
	uart1_init(115200);
	sys_Timer_Init();
	bsp_key_init();
	printf("start_03_fatfs_sd_demo\r\n");
	
	
	res_sd = f_mount(&fs,"0:",1);
	printf("KEYUP   : 格式化文件测试\r\n");
	printf("KEYDOWN : 写入文件测试\r\n");
	printf("KEYLEFT : 读出文件测试\r\n");

	while(1)
	{
		KEYS waitKey = Key_GetNum();
/*----------------------- 格式化测试 ---------------------------*/  		
		if(waitKey == KEY_UP)
		{
			/* 如果没有文件系统就格式化创建创建文件系统 */
			if(res_sd == FR_NO_FILESYSTEM)
			{
				printf("》SD卡还没有文件系统，即将进行格式化...\r\n");
				/* 格式化 */
				res_sd=f_mkfs("0:",0,0);							
				
				if(res_sd == FR_OK)
				{
					printf("》SD卡已成功格式化文件系统。\r\n");
					/* 格式化后，先取消挂载 */
					res_sd = f_mount(NULL,"0:",1);			
					/* 重新挂载	*/			
					res_sd = f_mount(&fs,"0:",1);
				}
				else
				{
					
					printf("《《格式化失败。》》\r\n");
					while(1);
				}
			}
			else if(res_sd!=FR_OK)
			  {
				printf("！！SD卡挂载文件系统失败。(%d)\r\n",res_sd);
				printf("！！可能原因：SD卡初始化不成功。\r\n");
					while(1);
			  }
			  else
			  {
				printf("》文件系统挂载成功，可以进行读写测试\r\n");
			  }		
			
		}
/*----------------------- 文件系统测试：写测试 -----------------------------*/
		if(waitKey == KEY_DOWN)
		{
			/* 打开文件，如果文件不存在则创建它 */
			printf("\r\n****** 即将进行文件写入测试... ******\r\n");	
			res_sd = f_open(&fnew, "0:FatFs读写测试文件.txt",FA_CREATE_ALWAYS | FA_WRITE );
			if ( res_sd == FR_OK )
			{
				printf("》打开/创建FatFs读写测试文件.txt文件成功，向文件写入数据。\r\n");
				/* 将指定存储区内容写入到文件内 */
				res_sd=f_write(&fnew,WriteBuffer,sizeof(WriteBuffer),&fnum);
			if(res_sd==FR_OK)
			{
			  printf("》文件写入成功，写入字节数据：%d\n",fnum);
			  printf("》向文件写入的数据为：\r\n%s\r\n",WriteBuffer);
			}
			else
			{
			  printf("！！文件写入失败：(%d)\n",res_sd);
			}    
				/* 不再读写，关闭文件 */
			f_close(&fnew);
			}
			else
			{	
				
				printf("！！打开/创建文件失败。\r\n");
			}		
		}
		
		
		if(waitKey == KEY_LEFT)
		{
/*------------------- 文件系统测试：读测试 ------------------------------------*/
			printf("****** 即将进行文件读取测试... ******\r\n");
			res_sd = f_open(&fnew, "0:FatFs读写测试文件.txt", FA_OPEN_EXISTING | FA_READ); 	 
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


		if(waitKey == KEY_RIGHT)
		{
		
			/* 不再使用文件系统，取消挂载文件系统 */
			f_mount(NULL,"0:",1);			
		
		}			
			
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
