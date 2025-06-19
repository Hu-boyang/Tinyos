#ifndef OS_CFG_H
#define OS_CFG_H

/// @brief gdt表的大小
#define GDT_TABLE_SIZE 256

/// @brief 设置内核代码段的索引
#define KERNEL_SELECTOR_CS (1*8) 

/// @brief 设置内核数据段索引
#define KERNEL_SELECTOR_DS (2*8) 

/// @brief 设置内核栈的大小
#define KERNEL_STACK_SIZE (8*1024)

/// @brief 定时器多长时间中断一次
#define OS_TICK_MS              10

/// @brief 内核的版本号
#define OS_VERSION              "1.0.0"

/// @brief 空闲任务的栈大小
#define IDLE_TASK_SIZE          1024

/// @brief 系统调用的选择子的索引
#define SELECTOR_SYSCALL        (3*8)

/// @brief 任务的数量
#define TASK_NR                 128

/// @brief 根文件系统的设备号
#define ROOT_DEV             DEV_DISK,0xb1
#endif