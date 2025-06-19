#ifndef FATFS_H
#define FATFS_H

#include "comm/types.h"

#pragma pack(1)

/// @brief 文件只读属性
#define DIRITEM_ATTR_READ_ONLY    0x01  // 只读属性

/// 文件隐藏属性
#define DIRITEM_ATTR_HIDDEN       0x02  // 隐藏属性

/// @brief 文件系统属性
#define DIRITEM_ATTR_SYSTEM       0x04  // 系统属性

/// @brief 文件卷标属性
#define DIRITEM_ATTR_VOLUME_ID    0x08  // 卷标属性

/// @brief 文件目录属性
#define DIRITEM_ATTR_DIRECTORY    0x10  // 目录属性

/// @brief 文件存档属性
#define DIRITEM_ATTR_ARCHIVE      0x20  // 存档属性

/// @brief 长文件名属性
#define DIRITEM_ATTR_LONG_NAME    0x0F  // 长文件名属性

/// @brief 文件名首字母为0xE5表示该文件已被删除
#define DIRITEM_NAME_FREE       0xE5  

/// @brief 文件名结束标志
#define DIRITEM_NAME_END       0x00  

/// @brief 无效簇号
#define FAT_CLUSTER_INVALID 0xFFF8

/// @brief FAT表中表示空闲簇的值
#define FAT_CLUSTER_FREE       0
 
/**
 * @brief diritem结构体描述
 * @param DIR_Name 文件名，11字节
 * @param DIR_Attr 文件属性，1字节
 * @param DIR_NTRes 保留字节，1字节
 * @param DIR_CrtTimeTenth 创建时间的毫秒部分，1字节
 * @param DIR_CrtTime 创建时间，2字节
 * @param DIR_CrtDate 创建日期，2字节
 * @param DIR_LstAccDate 最后访问日期，2字节
 * @param DIR_FstClusHI 高位簇号，2字节
 * @param DIR_WrtTime 最后写入时间，2字节
 * @param DIR_WrtDate 最后写入日期，2字节
 * @param DIR_FstClusLO 低位簇号，2字节
 * @param DIR_FileSize 文件大小（字节），4字节
 */
typedef struct _diritem_t{
    uint8_t DIR_Name[11];       // 文件名
    uint8_t DIR_Attr;           // 文件属性
    uint8_t DIR_NTRes;          // 保留字节
    uint8_t DIR_CrtTimeTenth;   // 创建时间的毫秒部分
    uint16_t DIR_CrtTime;       // 创建时间
    uint16_t DIR_CrtDate;       // 创建日期
    uint16_t DIR_LstAccDate;    // 最后访问日期
    uint16_t DIR_FstClusHI;     // 高位簇号
    uint16_t DIR_WrtTime;       // 最后写入时间
    uint16_t DIR_WrtDate;       // 最后写入日期
    uint16_t DIR_FstClusLO;     // 低位簇号
    uint32_t DIR_FileSize;      // 文件大小（字节）
}diritem_t;

/**
 * @brief dbr结构体描述
 * @param BS_jmpBoot 引导扇区跳转指令
 * @param BS_OMEName OEM名称
 * @param BPB_BytsPerSec 每个扇区的字节数
 * @param BPB_SecPerClus 每个簇的扇区数
 * @param BPB_RsvdSecCnt 从分区开始到第一个FAT表之间的扇区数量
 * @param BPB_NumFATs FAT表数量
 * @param BPB_RootEntCnt 根目录项数量
 * @param BPB_TotSec16 总扇区数（16位）
 * @param BPB_Media 媒体描述符
 * @param BPB_FATSz16 FAT表占用的扇区数量
 * @param BPB_SecPerTrk 每个磁道的扇区数
 * @param BPB_NumHeads 磁头数量
 * @param BPB_HiddSec 隐藏扇区数
 * @param BPB_TotSec32 总扇区数（32位）
 * @param BS_DrvNum 驱动器号
 * @param BS_Reserved1 保留字节
 * @param BS_BootSig 引导签名
 * @param BS_VolID 卷标ID
 * @param BS_VolLab 卷标
 * @param BS_FilSysType 文件系统类型
 */
typedef struct _dbr_t{
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OMEName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
}dbr_t;
#pragma pack()

/**
 * @brief 存储fat文件系统的相关信息
 * @param tbl_start FAT表起始扇区
 * @param tbl_cnt FAT表数量
 * @param tbl_sectors 每个FAT表占用扇区数
 * @param bytes_per_sec 每个扇区的字节数
 * @param sec_per_cluster 每个簇的扇区数
 * @param root_start 根目录起始扇区
 * @param root_ent_cnt 根目录项数量
 * @param data_start 数据区起始扇区
 * @param cluster_byte_size 簇的大小
 * @param fs 文件系统指针
 * @param fat_buff 用来存储扇区数据的缓冲区
 * @param curr_sector fat_buff存储的扇区的扇区号
 */
typedef struct _fat_t{
    uint32_t tbl_start;
    uint32_t tbl_cnt;
    uint32_t tbl_sectors;

    uint32_t bytes_per_sec;
    uint32_t sec_per_cluster;
    uint32_t root_start;
    uint32_t root_ent_cnt;
    uint32_t data_start;
    uint32_t cluster_byte_size;

    struct _fs_t *fs; // 文件系统指针

    uint8_t *fat_buff;

    int curr_sector;


}fat_t;

typedef uint16_t cluster_t;

int fatfs_mount(struct _fs_t *fs,int major,int minor);
#endif