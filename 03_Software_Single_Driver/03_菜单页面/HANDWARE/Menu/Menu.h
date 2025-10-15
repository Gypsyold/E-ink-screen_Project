#ifndef __Menu_H_
#define __Menu_H_
#include "Menu_Picture.h"
#include "Menu_Chinese.h"
#include "sys.h"

void EPD_InvertRect(u8 *image, u16 x, u16 y, u16 w, u16 h);

int menu1(void);
extern u8 ImageBW[];

#endif
