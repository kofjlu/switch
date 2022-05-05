/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-12-5      SummerGift   first version
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtthread.h>
#include <board.h>

#ifdef BSP_USING_SPI_FLASH_LITTLEFS
extern struct fal_flash_dev w25q128;
#endif

#define FLASH_SIZE_GRANULARITY_16K   (4 * 16 * 1024)
#define FLASH_SIZE_GRANULARITY_64K   (64 * 1024)
#define FLASH_SIZE_GRANULARITY_128K  (128 * 1024)
#define FLASH_SIZE_GRANULARITY_256K  (384 * 1024)
#define STM32_FLASH_START_ADRESS_16K  STM32_FLASH_START_ADRESS
#define STM32_FLASH_START_ADRESS_64K  (STM32_FLASH_START_ADRESS_16K + FLASH_SIZE_GRANULARITY_128K)
// #define STM32_FLASH_START_ADRESS_256K (STM32_FLASH_START_ADRESS_128K + FLASH_SIZE_GRANULARITY_128K)

extern const struct fal_flash_dev stm32_onchip_flash_16k;
extern const struct fal_flash_dev stm32_onchip_flash_64k;
#ifdef BSP_USING_OTA_DOWNLOADER
extern struct fal_flash_dev w25q128_DL;
extern struct fal_flash_dev w25q128_FY;
#endif
// extern const struct fal_flash_dev stm32_onchip_flash_128k;
// extern const struct fal_flash_dev stm32_onchip_flash_256k;




#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &w25q128,                                                        \
}
/*     &stm32_onchip_flash_16k,                                         \
    &stm32_onchip_flash_64k,                                         \ */
/*     &w25q128_DL,                                                     \
    &w25q128_FY,                                                     \ */

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG

/* partition table */
#define FAL_PART_TABLE                                                                                                     \
{                                                                                                                          \
    {FAL_PART_MAGIC_WROD, "spiflash0", "W25Q128", 0 , 12 * 1024 * 1024, 0}, \
}
/*     {FAL_PART_MAGIC_WROD, "bootloader", "onchip_flash_16k",  0 , FLASH_SIZE_GRANULARITY_128K , 0}, \
    {FAL_PART_MAGIC_WROD, "app",      "onchip_flash_64k",  0 , FLASH_SIZE_GRANULARITY_256K , 0}, \ */
/*     {FAL_PART_MAGIC_WROD, "download",        "w25q128_DL", 0 , FLASH_SIZE_GRANULARITY_256K, 0}, \
    {FAL_PART_MAGIC_WROD, "factory",        "w25q128_FY", 0 , FLASH_SIZE_GRANULARITY_256K, 0}, \ */
#endif /* FAL_PART_HAS_TABLE_CFG */
#endif /* _FAL_CFG_H_ */
