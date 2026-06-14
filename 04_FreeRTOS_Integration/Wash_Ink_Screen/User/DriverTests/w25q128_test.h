#ifndef W25Q128_TEST_H
#define W25Q128_TEST_H

/*
 * w25q128_test.h — W25Q128 驱动测试
 *
 * W25Q128_Test_Run() 在 FreeRTOS 任务中调用，依次执行：
 *   1. 读取 JEDEC ID 并验证
 *   2. 擦除测试扇区
 *   3. 页写入（256 字节，DMA 发送）
 *   4. 读取验证（与写入数据逐字节比较）
 *   5. 打印测试结果
 */

void W25Q128_Test_Run(void);

#endif /* W25Q128_TEST_H */
