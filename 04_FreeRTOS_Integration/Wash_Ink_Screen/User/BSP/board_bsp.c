#include "board_bsp.h"

/*
 * board_bsp.c — no executable code.
 *
 * SPI handles (hspi1, hspi2) are defined in Core/Src/main.c by CubeMX.
 * board_bsp.h extern-declares them so upper layers can reference them
 * without including main.h directly.
 *
 * GPIO initialization is done by MX_GPIO_Init() in main.c (CubeMX).
 */
