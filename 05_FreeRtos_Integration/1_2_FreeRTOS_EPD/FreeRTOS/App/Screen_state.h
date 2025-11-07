#ifndef _Screen_state_h_
#define _Screen_state_h_

#include "Key_RTOS.h"


void ScreenState_Init(void);
CurrentScreen ScreenState_Get(void);
void ScreenState_Set(CurrentScreen newScreen);

#endif

