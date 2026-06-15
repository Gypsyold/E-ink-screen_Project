#include "sd_card.h"
#include "fatfs.h"
#include "main.h"   /* SD_DET_Pin / SD_DET_GPIO_Port */

uint8_t SD_IsDetected(void)
{
    /* PD3 上拉输入：插卡后开关接地 → 低电平 = 已插入 */
    return (HAL_GPIO_ReadPin(SD_DET_GPIO_Port, SD_DET_Pin) == GPIO_PIN_RESET) ? 1u : 0u;
}

SD_Err SD_Mount(void)
{
    if (!SD_IsDetected()) return SD_ERR_NO_CARD;
    if (f_mount(&SDFatFS, SDPath, 1) != FR_OK) return SD_ERR_MOUNT;
    return SD_OK;
}

void SD_Unmount(void)
{
    f_mount(NULL, SDPath, 0);
}

SD_Err SD_ReadFile(const char *path, uint8_t *buf,
                   uint32_t buf_size, uint32_t *out_len)
{
    FIL     fil;
    UINT    br;
    FRESULT res;

    if (f_open(&fil, path, FA_READ) != FR_OK) return SD_ERR_NOT_FOUND;

    /* 留 1 字节给结尾 '\0' */
    res = f_read(&fil, buf, buf_size - 1u, &br);
    f_close(&fil);

    if (res != FR_OK) return SD_ERR_READ;

    buf[br] = 0x00u;
    if (out_len) *out_len = (uint32_t)br;
    return SD_OK;
}

SD_Err SD_FileSize(const char *path, uint32_t *out_size)
{
    FILINFO fno;
    if (f_stat(path, &fno) != FR_OK) return SD_ERR_NOT_FOUND;
    *out_size = (uint32_t)fno.fsize;
    return SD_OK;
}
