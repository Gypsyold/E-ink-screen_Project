#include "EPD.h"
#include "delay.h"



/*******************************************************************
		����˵��:��æ����
		��ڲ���:��
		˵��:æ״̬Ϊ1		
*******************************************************************/
void EPD_READBUSY(void)
{
	while(1)
	{
		if(EPD_ReadBusy==0)
		{
			break;
		}
	}
}

/*******************************************************************
		����˵��:Ӳ����λ����
		��ڲ���:��
		˵��:��E-Paper����Deepsleep״̬����ҪӲ����λ	
*******************************************************************/
void EPD_HW_RESET(void)
{
	delay_ms(100);
	EPD_RES_Clr();
	delay_ms(10);
	EPD_RES_Set();
	delay_ms(10);
	EPD_READBUSY();
}

/*******************************************************************
		����˵��:���º���
		��ڲ���:��	
		˵��:������ʾ���ݵ�E-Paper		
*******************************************************************/
void EPD_Update(void)
{
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0xF4);
	EPD_WR_REG(0x20);
	EPD_READBUSY();
}
/*******************************************************************
		����˵��:��ˢ���º���
		��ڲ���:��
		˵��:E-Paper�����ھ�ˢģʽ
*******************************************************************/
void EPD_PartUpdate(void)
{
	
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0x1C);
	EPD_WR_REG(0x20);
	EPD_READBUSY();
}
/*******************************************************************
		����˵��:��ˢ���º���
		��ڲ���:��
		˵��:E-Paper�����ڿ�ˢģʽ
*******************************************************************/
void EPD_FastUpdate(void)
{
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0xC7);
	EPD_WR_REG(0x20);
	EPD_READBUSY();
}

/*******************************************************************
		����˵��:���ߺ���
		��ڲ���:��
		˵��:��Ļ����͹���ģʽ		
*******************************************************************/
void EPD_DeepSleep(void)
{
	EPD_WR_REG(0x10);
	EPD_WR_DATA8(0x01);
	delay_ms(200);
}

/*******************************************************************
		����˵��:��ʼ������
		��ڲ���:��
		˵��:����E-PaperĬ����ʾ����
*******************************************************************/
void EPD_Init(void)
{
	EPD_HW_RESET();
	EPD_READBUSY();   
	EPD_WR_REG(0x12);  //SWRESET
	EPD_READBUSY();   
	
	EPD_WR_REG(0x3C); //BorderWavefrom
	EPD_WR_DATA8(0x05);
	
	EPD_WR_REG(0x01); //Driver output control      
	EPD_WR_DATA8((EPD_H-1)%256);    	//��8λ
	EPD_WR_DATA8((EPD_H-1)/256);		//��8λ
	EPD_WR_DATA8(0x00); 				//�������ײ��������Ҳ�

	EPD_WR_REG(0x11); //data entry mode       //�涨ͼ������д�� RAM ʱ�ĵ�ַ���������·���
	EPD_WR_DATA8(0x02);// 010	X �ݼ���Y ����
	EPD_WR_REG(0x44); //set Ram-X address start/end position   
	EPD_WR_DATA8(EPD_W/8-1);    
	EPD_WR_DATA8(0x00);  
	EPD_WR_REG(0x45); //set Ram-Y address start/end position          
	EPD_WR_DATA8(0x00);
	EPD_WR_DATA8(0x00); 
	EPD_WR_DATA8((EPD_H-1)%256); 
	EPD_WR_DATA8((EPD_H-1)/256);
	EPD_WR_REG(0x21); //  Display update control
	EPD_WR_DATA8(0x00);		
	EPD_WR_DATA8(0x80);	
	EPD_WR_REG(0x18); //Read built-in temperature sensor
	EPD_WR_DATA8(0x80);	
	EPD_WR_REG(0x4E);   // set RAM x address count to 0;
	EPD_WR_DATA8(EPD_W/8-1);    
	EPD_WR_REG(0x4F);   // set RAM y address count to 0X199;    
	EPD_WR_DATA8(0x00);
	EPD_WR_DATA8(0x00);
  EPD_READBUSY();
}

/*******************************************************************
		����˵��:��ˢ��ʼ������
		��ڲ���:��
		˵��:E-Paper�����ڿ�ˢģʽ
*******************************************************************/
void EPD_FastInit(void)
{
	EPD_HW_RESET();
	EPD_READBUSY();
//  EPD_WR_REG(0x12);  //��ת180����ʾ �������½�������λIC
//	EPD_READBUSY();
	EPD_WR_REG(0x18);
	EPD_WR_DATA8(0x80);
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0xB1);
	EPD_WR_REG(0x20);
	EPD_READBUSY();
	EPD_WR_REG(0x1A);
	EPD_WR_DATA8(0x64);
	EPD_WR_DATA8(0x00);
	EPD_WR_REG(0x22);
	EPD_WR_DATA8(0x91);
	EPD_WR_REG(0x20);
	EPD_READBUSY();
}


/*******************************************************************
		����˵��:��������
		��ڲ���:��
		˵��:E-Paperˢ����
*******************************************************************/
void EPD_Display_Clear(void)
{
  u16 i;
	EPD_WR_REG(0x3C);
	EPD_WR_DATA8(0x01);
	EPD_WR_REG(0x24);
	for(i=0;i<5624;i++)
	{
		EPD_WR_DATA8(0xFF);
	}	
	EPD_WR_REG(0x26);
	for(i=0;i<5624;i++)
	{
		EPD_WR_DATA8(0xFF);
	}	
}

/*******************************************************************
		����˵��:��ˢ����������
		��ڲ���:��
		˵��:E-Paper�����ھ�ˢģʽ�µ���
*******************************************************************/
void EPD_Clear_R26H(void)
{
	u16 i;
	EPD_READBUSY();
	EPD_WR_REG(0x26);
	for(i=0;i<5624;i++)
	{
		EPD_WR_DATA8(0xFF);
	}
}

/*******************************************************************
		����˵��:�������ݸ��µ�E-Paper
		��ڲ���:��
		˵��:
*******************************************************************/
void EPD_Display(const u8 *image)
{
	u16 i,j,Width,Height;
	Width=(EPD_W%8==0)?(EPD_W/8):(EPD_W/8+1);
	Height=EPD_H;
	EPD_WR_REG(0x24);
	for (j=0;j<Height;j++) 
	{
		for (i=0;i<Width;i++) 
		{
			EPD_WR_DATA8(image[i+j*Width]);
		}
	}
}



