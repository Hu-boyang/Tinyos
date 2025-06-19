#ifndef DISK_H
#define DISK_H

#include "comm/types.h"
#include "ipc/mutex.h"
#include "ipc/sem.h"

/// @brief 磁盘名称的大小
#define DISK_NAME_SIZE 32

/// @brief 分区名称的大小
#define PART_NAME_SIZE 32

/// @brief 分区数量
#define DISK_PRIMARY_PART_NR (4+1)

/// @brief 磁盘数量
#define DISK_CNT 2

/// @brief 定义IDE控制器能够连接的磁盘数量,IDE控制器是计算机连接控制器的一个接口标准
#define DISK_PER_CHANNEL 2

/// @brief 定义主IDE控制器I/O基地址
#define IOBASE_PRIMARY 0x1F0

/// @brief 获取硬盘数据寄存器的地址
#define DISK_DATA(disk) (disk->port_base+0)

/// @brief 获取硬盘错误寄存器的地址
#define DISK_ERR(disk) (disk->port_base+1)

/// @brief 获取硬盘扇区计数寄存器的地址
#define DISK_SECTOR_COUNT(disk) (disk->port_base+2)

/// @brief 获取扇区地址的低8位
#define DISK_LBA_LO(disk) (disk->port_base+3)

/// @brief 获取扇区地址的中8位
#define DISK_LBA_MID(disk) (disk->port_base+4)

/// @brief 获取扇区地址的高8位
#define DISK_LBA_HI(disk) (disk->port_base+5)

/// @brief 获取硬盘驱动器寄存器的地址
#define DISK_DRIVE(disk) (disk->port_base+6)

/// @brief 获取硬盘状态寄存器的地址
#define DISK_STATUS(disk) (disk->port_base+7)

/// @brief 获取硬盘命令寄存器的地址
#define DISK_CMD(disk) (disk->port_base+7)

/// @brief 磁盘状态为错误
#define DISK_STATUS_ERR (1 << 0)

/// @brief 磁盘状态为就绪
#define DISK_STATUS_DRQ (1 << 3)

/// @brief 磁盘状态为故障
#define DISK_STATUS_DF (1 << 5)

/// @brief 磁盘状态为忙
#define DISK_STATUS_BUSY (1 << 7)

/// @brief 识别磁盘命令，获取磁盘详细信息和参数
#define DISK_CMD_IDENTIFY (0xEC)

/// @brief 读取扇区命令
#define DISK_CMD_READ (0x24)

/// @brief 写入扇区命令
#define DISK_CMD_WRITE (0x34)

/// @brief IDE硬盘驱动器选择寄存器的基础值
#define DISK_DRIVE_BASE (0xE0)

/// @brief 分区表项数量
#define MBR_PRIMARY_PART_NR 4

// 这里的宏是为了能够让其对其为512个字节
#pragma pack(1)
/**
 * @brief 记录磁盘分区表表项的结构体
 * @param boot_active 分区是否活动，1表示活动，0表示不活动
 * @param start_header 分区起始头部号
 * @param start_sector 分区起始扇区号，6位
 * @param start_cylinder 分区起始柱面号，10位
 * @param system_id 分区系统ID，表示分区类型
 * @param end_header 分区结束头部号
 * @param end_sector 分区结束扇区号，6位
 * @param end_cylinder 分区结束柱面号，10位
 * @param realative_sectors LBA分区起始扇区号
 * @param total_sectors 分区总扇区数
 */
typedef struct _part_item_t{
    uint8_t boot_active;
    uint8_t start_header;
    uint16_t start_sector:6;
    uint16_t start_cylinder:10;
    uint8_t system_id;
    uint8_t end_header;
    uint16_t end_sector:6;
    uint16_t end_cylinder:10;
    uint32_t relative_sectors;
    uint32_t total_sectors;
}part_item_t;

/**
 * @brief 主引导记录结构体
 * @param code 主引导记录代码，大小为446字节
 * @param part_item 主引导记录分区表项，大小为64字节
 * @param boot_sig 主引导记录签名，大小为2字节，两个字节分别为0x55和0xAA
 */
typedef struct _mbr_t{
    uint8_t code[446];
    part_item_t part_item[MBR_PRIMARY_PART_NR];
    uint8_t boot_sig[2];
}mbr_t;

#pragma pack()

struct _disk_t;

/**
 * @brief 描述分区的结构体
 * @param name 分区名称
 * @param disk 磁盘指针，指向其该分区所在的磁盘
 * @param start_sector 分区起始扇区
 * @param total_sector 分区总扇区数
 * @param type 分区类型
 */
typedef struct _partinfo_t{
    char name[PART_NAME_SIZE];
    struct _disk_t* disk;
    
    enum{
        FS_INVALID=0x00,
        FS_FAT16_0=0x06,
        FS_FAT16_1=0x0E,
    }type;

    int start_sector;
    int total_sector;
}partinfo_t;

/**
 * @brief 描述磁盘的结构体
 * @param name 磁盘名称
 * @param sector_size 扇区大小
 * @param sector_count 扇区数量
 * @param partinfo 分区表
 * @param drive 磁盘驱动器类型
 * @param port_base 磁盘端口基址
 * @param partinfo 分区信息数组
 * @param mutex 磁盘互斥锁
 * @param op_sem 磁盘操作信号量
 */
typedef struct _disk_t{
    char name[DISK_NAME_SIZE];
    int sector_size;
    int sector_count;

    enum{
        DISK_MASTER=(0 << 4),
        DISK_SLAVE=(1 << 4),
    }drive;

    uint16_t port_base;
    partinfo_t partinfo[DISK_PRIMARY_PART_NR];

    mutex_t* mutex;
    sem_t* op_sem;
}disk_t;

void disk_init(void);

void exception_handler_ide_primary(void);
#endif