#ifndef EPD_TEST_H
#define EPD_TEST_H

/*
 * epd_test.h — EPD driver verification tests.
 * Call EPD_Test() from a FreeRTOS task after SPI_BSP_Init() completes.
 */

void EPD_Test(void);

#endif /* EPD_TEST_H */
