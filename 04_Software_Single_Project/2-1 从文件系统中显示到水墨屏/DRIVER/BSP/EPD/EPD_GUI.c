#include "EPD_GUI.h"
#include "EPD_Font.h"
#include "spi_w25Q128_flash.h"
#include <stdio.h>

PAINT Paint;


/*******************************************************************
		函数说明：创建图片缓存数组
		接口说明：*image  要传入的图片数组
               Width  图片宽度
               Heighe 图片长度
               Rotate 屏幕显示方向
               Color  显示颜色
		返回值：  无
*******************************************************************/
void Paint_NewImage(u8 *image,u16 Width,u16 Height,u16 Rotate,u16 Color)
{
	Paint.Image = 0x00;
	Paint.Image = image;
	Paint.color = Color;  
	Paint.widthMemory = Width;
	Paint.heightMemory = Height;  
	Paint.widthByte = (Width % 8 == 0)? (Width / 8 ): (Width / 8 + 1);
	Paint.heightByte = Height;     
	Paint.rotate = Rotate;
	if(Rotate==0||Rotate==180) 
	{
		Paint.width=Height;
		Paint.height=Width;
	} 
	else 
	{
		Paint.width = Width;
		Paint.height = Height;
	}
}				 

/*******************************************************************
		函数说明：清空缓冲区 
		接口说明：Color  像素点颜色参数
		返回值：  无
*******************************************************************/
void Paint_Clear(u8 Color)
{
	
	u16 X,Y;
	u32 Addr;
	for(Y=0;Y<Paint.heightByte;Y++) 
	{
		for(X=0;X<Paint.widthByte;X++) 
		{   
			Addr=X+Y*Paint.widthByte;//8 pixel =  1 byte
			Paint.Image[Addr]=Color;
		}
	 }
}


/*******************************************************************
		函数说明：点亮一个像素点
		接口说明：Xpoint 像素点x坐标参数
              Ypoint 像素点Y坐标参数
              Color  像素点颜色参数
		返回值：  无
*******************************************************************/
void Paint_SetPixel(u16 Xpoint,u16 Ypoint,u16 Color)
{
	u16 X, Y;
	u32 Addr;
	u8 Rdata;
    switch(Paint.rotate) 
		{
			case 0:
					X=Ypoint;
					Y=Xpoint;		
					break;
			case 90:
					X=Xpoint;
					Y=Paint.heightMemory-Ypoint-1;
					break;
			case 180:
					X=Paint.widthMemory-Ypoint-1;
					Y=Paint.heightMemory-Xpoint-1;
					break;
			case 270:
					X=Paint.widthMemory-Xpoint-1;
					Y=Ypoint;
					break;
				default:
						return;
    }
		Addr=X/8+Y*Paint.widthByte;
    Rdata=Paint.Image[Addr];
    if(Color==BLACK)
    {    
			Paint.Image[Addr]=Rdata&~(0x80>>(X % 8)); //将对应数据位置0
		}
    else
		{
      Paint.Image[Addr]=Rdata|(0x80>>(X % 8));   //将对应数据位置1  
		}
}


/*******************************************************************
		函数说明：划线函数
		接口说明：Xstart 像素x起始坐标参数
              Ystart 像素Y起始坐标参数
							Xend   像素x结束坐标参数
              Yend   像素Y结束坐标参数
              Color  像素点颜色参数
		返回值：  无
*******************************************************************/
void EPD_DrawLine(u16 Xstart,u16 Ystart,u16 Xend,u16 Yend,u16 Color)
{   
	u16 Xpoint, Ypoint;
	int dx, dy;
	int XAddway,YAddway;
	int Esp;
	char Dotted_Len;
  Xpoint = Xstart;
  Ypoint = Ystart;
  dx = (int)Xend - (int)Xstart >= 0 ? Xend - Xstart : Xstart - Xend;
  dy = (int)Yend - (int)Ystart <= 0 ? Yend - Ystart : Ystart - Yend;
  XAddway = Xstart < Xend ? 1 : -1;
  YAddway = Ystart < Yend ? 1 : -1;
  Esp = dx + dy;
  Dotted_Len = 0;
  for (;;) {
        Dotted_Len++;
            Paint_SetPixel(Xpoint, Ypoint, Color);
        if (2 * Esp >= dy) {
            if (Xpoint == Xend)
                break;
            Esp += dy;
            Xpoint += XAddway;
        }
        if (2 * Esp <= dx) {
            if (Ypoint == Yend)
                break;
            Esp += dx;
            Ypoint += YAddway;
        }
    }
}
/*******************************************************************
		函数说明：画矩形函数
		接口说明：Xstart 矩形x起始坐标参数
              Ystart 矩形Y起始坐标参数
							Xend   矩形x结束坐标参数
              Yend   矩形Y结束坐标参数
              Color  像素点颜色参数
              mode   矩形是否进行填充
		返回值：  无
*******************************************************************/
void EPD_DrawRectangle(u16 Xstart,u16 Ystart,u16 Xend,u16 Yend,u16 Color,u8 mode)
{
	u16 i;
    if (mode)
			{
        for(i = Ystart; i < Yend; i++) 
				{
          EPD_DrawLine(Xstart,i,Xend,i,Color);
        }
      }
		else 
		 {
        EPD_DrawLine(Xstart, Ystart, Xend, Ystart, Color);
        EPD_DrawLine(Xstart, Ystart, Xstart, Yend, Color);
        EPD_DrawLine(Xend, Yend, Xend, Ystart, Color);
        EPD_DrawLine(Xend, Yend, Xstart, Yend, Color);
		 }
}
/*******************************************************************
		函数说明：画圆函数
		接口说明：X_Center 圆心x起始坐标参数
              Y_Center 圆心Y坐标参数
							Radius   圆形半径参数
              Color  像素点颜色参数
              mode   圆形是否填充显示
		返回值：  无
*******************************************************************/
void EPD_DrawCircle(u16 X_Center,u16 Y_Center,u16 Radius,u16 Color,u8 mode)
{
	int Esp, sCountY;
	u16 XCurrent, YCurrent;
  XCurrent = 0;
  YCurrent = Radius;
  Esp = 3 - (Radius << 1 );
    if (mode) {
        while (XCurrent <= YCurrent ) { //Realistic circles
            for (sCountY = XCurrent; sCountY <= YCurrent; sCountY ++ ) {
                Paint_SetPixel(X_Center + XCurrent, Y_Center + sCountY, Color);//1
                Paint_SetPixel(X_Center - XCurrent, Y_Center + sCountY, Color);//2
                Paint_SetPixel(X_Center - sCountY, Y_Center + XCurrent, Color);//3
                Paint_SetPixel(X_Center - sCountY, Y_Center - XCurrent, Color);//4
                Paint_SetPixel(X_Center - XCurrent, Y_Center - sCountY, Color);//5
                Paint_SetPixel(X_Center + XCurrent, Y_Center - sCountY, Color);//6
                Paint_SetPixel(X_Center + sCountY, Y_Center - XCurrent, Color);//7
                Paint_SetPixel(X_Center + sCountY, Y_Center + XCurrent, Color);
            }
            if ((int)Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    } else { //Draw a hollow circle
        while (XCurrent <= YCurrent ) {
            Paint_SetPixel(X_Center + XCurrent, Y_Center + YCurrent, Color);//1
            Paint_SetPixel(X_Center - XCurrent, Y_Center + YCurrent, Color);//2
            Paint_SetPixel(X_Center - YCurrent, Y_Center + XCurrent, Color);//3
            Paint_SetPixel(X_Center - YCurrent, Y_Center - XCurrent, Color);//4
            Paint_SetPixel(X_Center - XCurrent, Y_Center - YCurrent, Color);//5
            Paint_SetPixel(X_Center + XCurrent, Y_Center - YCurrent, Color);//6
            Paint_SetPixel(X_Center + YCurrent, Y_Center - XCurrent, Color);//7
            Paint_SetPixel(X_Center + YCurrent, Y_Center + XCurrent, Color);//0
            if ((int)Esp < 0 )
                Esp += 4 * XCurrent + 6;
            else {
                Esp += 10 + 4 * (XCurrent - YCurrent );
                YCurrent --;
            }
            XCurrent ++;
        }
    }
}

/******************************************************************************
      函数说明：显示汉字串
      入口数据：x,y显示坐标
                *s 要显示的汉字串
                sizey 字号 
                color 文字颜色
      返回值：  无
******************************************************************************/
void EPD_ShowChinese(u16 x,u16 y,u8 *s,u8 sizey,u16 color)
{
	while(*s!=0)
	{
		if(sizey==12) EPD_ShowChinese12x12(x,y,s,sizey,color);
		else if(sizey==16) EPD_ShowChinese16x16(x,y,s,sizey,color);
		else if(sizey==24) EPD_ShowChinese24x24(x,y,s,sizey,color);
		else if(sizey==32) EPD_ShowChinese32x32(x,y,s,sizey,color);
		else return;
		s+=2;
		x+=sizey;
	}
}

/******************************************************************************
      函数说明：显示单个12x12汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                sizey 字号
                color 文字颜色
      返回值：  无
******************************************************************************/
void EPD_ShowChinese12x12(u16 x,u16 y,u8 *s,u8 sizey,u16 color)
{
	u8 i,j;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;                    
	HZnum=sizeof(tfont12)/sizeof(typFNT_GB12);	//统计汉字数目
	for(k=0;k<HZnum;k++) 
	{
		if((tfont12[k].Index[0]==*(s))&&(tfont12[k].Index[1]==*(s+1)))
		{ 	
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
						if(tfont12[k].Msk[i]&(0x01<<j))	Paint_SetPixel(x,y,color);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
				}
			}
		}				  	
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
} 

/******************************************************************************
      函数说明：显示单个16x16汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                sizey 字号
                color 文字颜色
      返回值：  无
******************************************************************************/
void EPD_ShowChinese16x16(u16 x,u16 y,u8 *s,u8 sizey,u16 color)
{
	u8 i,j;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont16)/sizeof(typFNT_GB16);	//统计汉字数目
	for(k=0;k<HZnum;k++) 
	{
		if ((tfont16[k].Index[0]==*(s))&&(tfont16[k].Index[1]==*(s+1)))
		{ 	
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
						if(tfont16[k].Msk[i]&(0x01<<j))	Paint_SetPixel(x,y,color);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
				}
			}
		}				  	
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
} 


/******************************************************************************
      函数说明：显示单个24x24汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                sizey 字号
                color 文字颜色
      返回值：  无
******************************************************************************/
void EPD_ShowChinese24x24(u16 x,u16 y,u8 *s,u8 sizey,u16 color)
{
	u8 i,j;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont24)/sizeof(typFNT_GB24);	//统计汉字数目
	for(k=0;k<HZnum;k++) 
	{
		if ((tfont24[k].Index[0]==*(s))&&(tfont24[k].Index[1]==*(s+1)))
		{ 	
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
					if(tfont24[k].Msk[i]&(0x01<<j))	Paint_SetPixel(x,y,color);//画一个点
					x++;
					if((x-x0)==sizey)
					{
						x=x0;
						y++;
						break;
					}
				}
			}
		}				  	
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
} 

/******************************************************************************
      函数说明：显示单个32x32汉字
      入口数据：x,y显示坐标
                *s 要显示的汉字
                sizey 字号
                color 文字颜色
      返回值：  无
******************************************************************************/
void EPD_ShowChinese32x32(u16 x,u16 y,u8 *s,u8 sizey,u16 color)
{
	u8 i,j;
	u16 k;
	u16 HZnum;//汉字数目
	u16 TypefaceNum;//一个字符所占字节大小
	u16 x0=x;
	TypefaceNum=(sizey/8+((sizey%8)?1:0))*sizey;
	HZnum=sizeof(tfont32)/sizeof(typFNT_GB32);	//统计汉字数目
	for(k=0;k<HZnum;k++) 
	{
		if ((tfont32[k].Index[0]==*(s))&&(tfont32[k].Index[1]==*(s+1)))
		{ 	
			for(i=0;i<TypefaceNum;i++)
			{
				for(j=0;j<8;j++)
				{	
						if(tfont32[k].Msk[i]&(0x01<<j))	Paint_SetPixel(x,y,color);//画一个点
						x++;
						if((x-x0)==sizey)
						{
							x=x0;
							y++;
							break;
						}
				}
			}
		}				  	
		continue;  //查找到对应点阵字库立即退出，防止多个汉字重复取模带来影响
	}
}


/*******************************************************************
		函数说明：显示单个字符
		接口说明：x 		 字符x坐标参数
              y 		 字符Y坐标参数
							chr    要显示的字符
              size1  显示字符字号大小
              Color  像素点颜色参数
		返回值：  无
*******************************************************************/
void EPD_ShowChar(u16 x,u16 y,u16 chr,u16 size1,u16 color)
{
	u16 i,m,temp,size2,chr1;
	u16 x0,y0;
	x0=x,y0=y;
	if(size1==8)size2=6;
	else size2=(size1/8+((size1%8)?1:0))*(size1/2);  //得到字体一个字符对应点阵集所占的字节数
	chr1=chr-' ';  //计算偏移后的值
	for(i=0;i<size2;i++)
	{
		if(size1==8)
			  {temp=asc2_0806[chr1][i];} //调用0806字体
		else if(size1==12)
        {temp=asc2_1206[chr1][i];} //调用1206字体
		else if(size1==16)
        {temp=asc2_1608[chr1][i];} //调用1608字体
		else if(size1==24)
        {temp=asc2_2412[chr1][i];} //调用2412字体
		else if(size1==48)
        {temp=asc2_4824[chr1][i];} //调用2412字体
		else return;
		for(m=0;m<8;m++)
		{
			if(temp&0x01)Paint_SetPixel(x,y,color);
			else Paint_SetPixel(x,y,!color);
			temp>>=1;
			y++;
		}
		x++;
		if((size1!=8)&&((x-x0)==size1/2))
		{x=x0;y0=y0+8;}
		y=y0;
  }
}

/*******************************************************************
		函数说明：显示字符串
		接口说明：x 		 字符串x坐标参数
              y 		 字符串Y坐标参数
							*chr    要显示的字符串
              size1  显示字符串字号大小
              Color  像素点颜色参数
		返回值：  无
*******************************************************************/
void EPD_ShowString(u16 x,u16 y,u8 *chr,u16 size1,u16 color)
{
	while(*chr!='\0')//判断是不是非法字符!
	{
		EPD_ShowChar(x,y,*chr,size1,color);
		chr++;
		x+=size1/2;
  }
}
/*******************************************************************
		函数说明：指数运算
		接口说明：m 底数
              n 指数
		返回值：  m的n次方
*******************************************************************/
u32 EPD_Pow(u16 m,u16 n)
{
	u32 result=1;
	while(n--)
	{
	  result*=m;
	}
	return result;
}
/*******************************************************************
		函数说明：显示整型数字
		接口说明：x 		 数字x坐标参数
              y 		 数字Y坐标参数
							num    要显示的数字
              len    数字的位数
              size1  显示字符串字号大小
              Color  像素点颜色参数
		返回值：  无
*******************************************************************/
void EPD_ShowNum(u16 x,u16 y,u32 num,u16 len,u16 size1,u16 color)
{
	u8 t,temp,m=0;
	if(size1==8)m=2;
	for(t=0;t<len;t++)
	{
		temp=(num/EPD_Pow(10,len-t-1))%10;
			if(temp==0)
			{
				EPD_ShowChar(x+(size1/2+m)*t,y,'0',size1,color);
      }
			else 
			{
			  EPD_ShowChar(x+(size1/2+m)*t,y,temp+'0',size1,color);
			}
  }
}
/*******************************************************************
		函数说明：显示浮点型数字
		接口说明：x 		 数字x坐标参数
              y 		 数字Y坐标参数
							num    要显示的浮点数
              len    数字的位数
              pre    浮点数的精度
              size1  显示字符串字号大小
              Color  像素点颜色参数
		返回值：  无
*******************************************************************/

void EPD_ShowFloatNum1(u16 x,u16 y,float num,u8 len,u8 pre,u8 sizey,u8 color)
{         	
	u8 t,temp,sizex;
	u16 num1;
	sizex=sizey/2;
	num1=num*EPD_Pow(10,pre);
	for(t=0;t<len;t++)
	{
		temp=(num1/EPD_Pow(10,len-t-1))%10;
		if(t==(len-pre))
		{
			EPD_ShowChar(x+(len-pre)*sizex,y,'.',sizey,color);
			t++;
			len+=1;
		}
	 	EPD_ShowChar(x+t*sizex,y,temp+48,sizey,color);
	}
}





//GUI显示秒表
void EPD_ShowWatch(u16 x,u16 y,float num,u8 len,u8 pre,u8 sizey,u8 color)
{         	
	u8 t,temp,sizex;
	u16 num1;
	sizex=sizey/2;
	num1=num*EPD_Pow(10,pre);
	for(t=0;t<len;t++)
	{
		temp=(num1/EPD_Pow(10,len-t-1))%10;
		if(t==(len-pre))
		{
			EPD_ShowChar(x+(len-pre)*sizex+(sizex/2-2),y-6,':',sizey,color);
			t++;
			len+=1;
		}
	 	EPD_ShowChar(x+t*sizex,y,temp+48,sizey,color);
	}
}



void EPD_ShowPicture(u16 x,u16 y,u16 sizex,u16 sizey,const u8 BMP[],u16 Color)
{
	u16 j=0,t;
	u16 i,n,temp;
	u16 y0;
	y0=y;
	sizex=sizex/8+((sizex%8)?1:0);
	for(n=0;n<sizey;n++)
	{
		for(i=0;i<sizex;i++)
		{
			temp=BMP[j];
			for(t=0;t<8;t++)
			{
			  if(temp&0x80)
			  {
				  Paint_SetPixel(x,y,!Color);
		   	}
			  else
			  {
				  Paint_SetPixel(x,y,Color);
		  	}
			  y++;
				temp<<=1;
			}
			if((y-y0)==sizey)
			{
				y=y0;
				x++;
			}
			j++;
		}
	}
}







// 字体参数表（新增字体时，在此处添加对应参数）
static const Font_Param font_param_table[] = 
{
    [FONT_16X16] = {16, 16, 32},  // 16×16=32字节
    [FONT_24X24] = {24, 24, 72},  // 24×24=72字节（24*24/8=72）
    [FONT_32X32] = {32, 32, 128}  // 32×32=128字节（32*32/8=128）
};

/***********************************************************************************************
 * 函 数 名 称：EPD_show_Chinese_from_flash
 * 函 数 功 能：动态字体大小的汉字显示
 * 传 入 参 数：x=水墨屏横坐标 y=水墨屏纵坐标 *s=字符 font_size=显示大小 color=字体颜色
 * 函 数 返 回：无
 * 作       者：雪碧的情人
 * 备       注：字体大小按Font_Size结构体来
************************************************************************************************/
void EPD_show_Chinese_from_flash(u16 x, u16 y, u8 *s, Font_Size font_size, u16 color)
{
    u8 i, j;
    u16 x0 = x;
    uint8_t GBKH, GBKL;
    uint32_t Addr_offset;          									// 地址偏移
    const Font_Param *font = &font_param_table[font_size];  		// 获取当前字体参数
    uint8_t pBuff[128] = {0};      									// 最大支持32×32字体（128字节），兼容所有字体

    // 1. 校验字体大小有效性（防止传入无效枚举）
    if (font_size >= sizeof(font_param_table)/sizeof(Font_Param)) 
	{
        printf("错误：无效的字体大小！\n");
        return;
    }

    // 2. 提取GBK双字节编码
    GBKH = *(s);     												// 高字节
    GBKL = *(s + 1); 												// 低字节
    printf("GBKH=0x%x, GBKL=0x%x, 字体大小=%dx%d\n", GBKH, GBKL, font->width, font->height);
	
    // 3. 校验GBK编码有效性（不变）
    if (GBKH < 0x81 || GBKH > 0xFE || 
        GBKL < 0x40 || GBKL > 0xFE || 
        GBKL == 0x7F)
    {
        printf("错误：无效的GBK编码！\n");
        return;
    }

    // 4. 计算地址偏移
    if (GBKL < 0x7F)
    {
        Addr_offset = ((GBKH - 0x81) * 190 + (GBKL - 0x40));
    }
    else
    {
        Addr_offset = ((GBKH - 0x81) * 190 + (GBKL - 0x41));
    }
    printf("Addr_offset=%d, 字模地址=0x%X\n", Addr_offset, W25Q128_GBK_ADDR + Addr_offset * font->bytes);

    // 5. 从W25Q128读取偏移地址的字模，存储的时候按顺序存储，读出也按顺序读出
    W25Q128_read(pBuff, W25Q128_GBK_ADDR + Addr_offset * font->bytes, font->bytes);
	
	

    // 6. 逐点显示，显示到水墨屏数组上
    for (i = 0; i < font->bytes; i++)  								// 遍历字模所有字节
    {
        for (j = 0; j < 8; j++)       							 	// 遍历字节的8个bit
        {
            if (pBuff[i] & (0x01 << j))  							// LSB位序（bit0对应当前点）
            {
                Paint_SetPixel(x, y, color);
            }
            x++;  													// 横坐标递增（逐点显示）

																	// 一行显示完（达到字体宽度），换行
            if ((x - x0) >= font->width)  
            {
                x = x0;    											// 横坐标复位
                y++;       											// 纵坐标递增
                break;     											// 跳出当前字节，处理下一行
            }
        }
    }
}

/***********************************************************************************************
 * 函 数 名 称：EPD_ShowChineseString_flash
 * 函 数 功 能：连续显示汉字字符串
 * 传 入 参 数：x=水墨屏横坐标 y=水墨屏纵坐标 *str=字符串 font_size=显示大小 color=字体颜色
 * 函 数 返 回：无
 * 作       者：雪碧的情人
 * 备       注：字体大小按Font_Size结构体来
************************************************************************************************/
void EPD_ShowChineseString_flash(u16 x, u16 y, u8 *str, Font_Size font_size, u16 color)
{
    u16 curr_x = x;
    const Font_Param *font = &font_param_table[font_size];

    // 校验字体大小有效性
    if (font_size >= sizeof(font_param_table)/sizeof(Font_Param)) 
	{
        printf("错误：无效的字体大小！\n");
        return;
    }

    // 按“2字节一个汉字”遍历字符串（以'\0'结尾）
    while (*str != '\0' && *(str + 1) != '\0')
    {
        EPD_show_Chinese_from_flash(curr_x, y, str, font_size, color);
        curr_x += font->width;  // 按字体宽度偏移x坐标（不同字体间距不同）
        str += 2;               // 跳过当前汉字的2字节，取下一个
    }
}

/***********************************************************************************************
 * 合并版：自动区分GBK/ASCII
 * 传入参数：
 *   x/y        - 起始坐标
 *   buf        - FatFs读取的ReadBuffer
 *   font_size  - 汉字字体大小（如FONT_24X24）
 *   ascii_size - ASCII字符大小（如24）
 *   color      - 字体颜色
 ************************************************************************************************/
/**
 * @brief  在电子纸屏显示混合ASCII和中文的字符串（带完整边界保护）
 * @param  x: 起始X坐标(px)
 * @param  y: 起始Y坐标(px)
 * @param  buf: 待显示字符串（GBK编码，ASCII单字节/中文双字节）
 * @param  font_size: 字体大小索引（关联font_param_table）
 * @param  ascii_size: ASCII显示尺寸（实际宽度=ascii_size/2）
 * @param  color: 显示颜色（电子纸黑白对应值）
 * @retval 无
 */
void EPD_ShowMixedString(u16 x, u16 y, u8 *buf, Font_Size font_size, u16 ascii_size, u16 color)
{
    u16 i = 0;
    u16 curr_x = x;
    u16 line_num = 0; 
    const Font_Param *font = &font_param_table[font_size];
    u8 ascii_buf[32] = {0}; 
    u16 ascii_idx = 0;
    u16 line_height = font->height; // 单行文/ASCII高度（如24px）
    
    // ====================== 核心优化：统一屏幕参数为变量（替代宏定义） ======================
    // 屏幕基础参数（建议迁移到硬件配置文件，如epd.h，此处内联定义保持完整性）
    u16 screen_width = 296;         // 屏幕宽度（px）
    u16 screen_height = 144;        // 屏幕总高度（px），和宽度保持一致的变量形式
    u16 screen_max_lines = screen_height / line_height; // 动态计算最大行数
    
    u16 ascii_width = ascii_size / 2; // ASCII字符宽度（如12px）
    u16 chinese_width = font->width;  // 中文字符宽度（如24px）
    u8 is_out_of_height = 0;         // 纵向越界标记（避免重复判断）

    // 参数有效性校验（保留原逻辑，新增屏幕参数合理性校验）
    if (font_size >= sizeof(font_param_table)/sizeof(Font_Param) || buf == NULL)
    {
        printf("错误：参数无效！font_size越界或buf为空\n");
        return;
    }
    // 新增：屏幕参数合理性校验（避免行高为0导致除零错误）
    if (line_height == 0 || screen_height == 0)
    {
        printf("错误：屏幕高度或行高为0，无法计算最大行数\n");
        return;
    }

    // 从起始Y坐标计算初始行号（保留原逻辑）
    line_num = y / line_height;

    // ====================== 核心逻辑：遍历字符串并显示 ======================
    while (i < 256)
    {
        // 1. 先判断纵向是否已越界（前置预判，解决滞后性）
        if (line_num >= screen_max_lines)
        {
            printf("警告：当前行号(%d)超出屏幕最大行数(%d)，终止显示\n", line_num, screen_max_lines);
            is_out_of_height = 1; // 标记纵向越界
            break;
        }

        // 2. 字符串结束符判断（保留原逻辑）
        if (buf[i] == 0 && (i+1 >= 256 || buf[i+1] == 0)) 
        {
            printf("提示：已遍历到字符串末尾，退出循环\n");
            break;
        }

        // 3. 换行符处理（0x0D=回车/0x0A=换行）
        if (buf[i] == 0x0D || buf[i] == 0x0A)
        {
            // 先显示缓存的ASCII字符（保留原逻辑）
            if (ascii_idx > 0)
            {
                ascii_buf[ascii_idx] = '\0';
                EPD_ShowString(curr_x, line_num*line_height, ascii_buf, ascii_size, color);
                curr_x += (ascii_idx * ascii_width);
                ascii_idx = 0;
            }

            // 换行前预判纵向越界（解决滞后性）
            if (line_num + 1 >= screen_max_lines)
            {
                printf("警告：换行后行号(%d)超出屏幕最大行数(%d)，终止显示\n", line_num+1, screen_max_lines);
                is_out_of_height = 1;
                break;
            }
            line_num++; // 行号+1（换行）
            curr_x = x; // 重置X坐标到起始位置

            // 跳过连续换行符（保留原逻辑）
            while (i < 256 && (buf[i] == 0x0D || buf[i] == 0x0A)) i++;
            continue;
        }

        // 4. 处理ASCII字符（单字节，<=0x7F）
        if (buf[i] <= 0x7F && buf[i] != 0)
        {
            // 横向溢出判断：剩余宽度不足放下1个ASCII，换行（保留原逻辑）
            if (curr_x + ascii_width > screen_width)
            {
                // 换行前预判纵向越界
                if (line_num + 1 >= screen_max_lines)
                {
                    printf("警告：ASCII换行后行号(%d)超出屏幕最大行数(%d)，终止显示\n", line_num+1, screen_max_lines);
                    is_out_of_height = 1;
                    break;
                }
                line_num++;
                curr_x = x;
            }

            // 存入ASCII缓冲区（保留原逻辑）
            ascii_buf[ascii_idx++] = buf[i];
            i++;

            // 缓冲区满/遇到非ASCII，批量显示（保留原逻辑）
            if (ascii_idx >= 31 || (i < 256 && !(buf[i] <= 0x7F && buf[i] != 0)))
            {
                ascii_buf[ascii_idx] = '\0';
                EPD_ShowString(curr_x, line_num*line_height, ascii_buf, ascii_size, color);
                curr_x += (ascii_idx * ascii_width);
                ascii_idx = 0;
            }
        }
        // 5. 处理中文字符（双字节，0x81~0xFE）
        else if ((buf[i] >= 0x81 && buf[i] <= 0xFE) && (i+1 < 256 && buf[i+1] != 0))
        {
            // 先显示缓存的ASCII（保留原逻辑）
            if (ascii_idx > 0)
            {
                ascii_buf[ascii_idx] = '\0';
                EPD_ShowString(curr_x, line_num*line_height, ascii_buf, ascii_size, color);
                curr_x += (ascii_idx * ascii_width);
                ascii_idx = 0;
            }

            // 横向溢出判断：剩余宽度不足放下1个中文，换行（保留原逻辑）
            if (curr_x + chinese_width > screen_width)
            {
                // 换行前预判纵向越界
                if (line_num + 1 >= screen_max_lines)
                {
                    printf("警告：中文换行后行号(%d)超出屏幕最大行数(%d)，终止显示\n", line_num+1, screen_max_lines);
                    is_out_of_height = 1;
                    break;
                }
                line_num++;
                curr_x = x;
            }

            // 显示中文并更新坐标（保留原逻辑）
            EPD_show_Chinese_from_flash(curr_x, line_num*line_height, &buf[i], font_size, color);
            curr_x += chinese_width;
            i += 2;
        }
        else
        {
            i++; // 无效字符，跳过
        }

        // 原循环内纵向保护（双重保险）
        if (line_num >= screen_max_lines)
        {
            printf("警告：内容超出屏幕高度！\n");
            is_out_of_height = 1;
            break;
        }
    }

    // ====================== 最后一段ASCII完整纵向校验 ======================
    // 显示最后一段ASCII（仅当未纵向越界且缓冲区有内容时执行）
    if (ascii_idx > 0 && !is_out_of_height)
    {
        // 第一步：前置纵向校验（避免越界显示）
        if (line_num >= screen_max_lines)
        {
            printf("警告：最后一段ASCII行号(%d)超出屏幕最大行数(%d)，放弃显示\n", line_num, screen_max_lines);
            ascii_idx = 0;
        }
        else
        {
            // 第二步：横向溢出判断（保留原逻辑）
            if (curr_x + (ascii_idx * ascii_width) > screen_width)
            {
                // 换行前先判断是否越界（核心：解决滞后性）
                if (line_num + 1 >= screen_max_lines)
                {
                    printf("警告：最后一段ASCII换行后行号(%d)超出屏幕最大行数(%d)，放弃显示\n", line_num+1, screen_max_lines);
                    ascii_idx = 0;
                }
                else
                {
                    line_num++; // 换行
                    curr_x = x; // 重置X坐标
                }
            }

            // 第三步：最终校验+显示（确保万无一失）
            if (ascii_idx > 0 && line_num < screen_max_lines)
            {
                ascii_buf[ascii_idx] = '\0'; // 加字符串结束符
                EPD_ShowString(curr_x, line_num*line_height, ascii_buf, ascii_size, color);
                printf("提示：最后一段ASCII显示完成，位置(X:%d,Y:%d)\n", curr_x, line_num*line_height);
            }
        }
    }
    else if (ascii_idx > 0 && is_out_of_height)
    {
        printf("警告：纵向已越界，最后一段ASCII(%d个字符)放弃显示\n", ascii_idx);
        ascii_idx = 0;
    }
}
