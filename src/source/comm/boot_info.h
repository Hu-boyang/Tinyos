#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include "types.h"

/// @brief 定义内存信息的最大个数
#define BOOT_RAM_REGION_MAX			10		

/**
 * @brief 存储内存信息
 * @param ram_region_cfg 存储内存信息的数组包含start存储起始地址以及size即这块儿内存大小
 * @param ram_region_count 存储内存信息的个数
 */
typedef struct _boot_info_t {
    struct {
        uint32_t start;
        uint32_t size;
    }ram_region_cfg[BOOT_RAM_REGION_MAX];
    int ram_region_count;
}boot_info_t;

#define SECTOR_SIZE 512
#define SYS_KERNEL_LOAD_ADDR (1024*1024)
#endif
