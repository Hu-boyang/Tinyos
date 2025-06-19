#ifndef FILE_H
#define FILE_H

#include "comm/types.h"

/// @brief 文件名的大小
#define FILE_NAME_SIZE  32 

/// @brief 文件表的大小
#define FILE_TABLE_SIZE  2048

/**
 * @brief 文件类型
 * @param FILE_UNKNOWN 未知文件类型
 * @param FILE_TYPE_TTY tty设备文件
 * @param FILE_DIR 目录文件
 * @param FILE_NORMAL 普通文件
 */
typedef enum _file_type_t{
    FILE_UNKNOWN=0, 
    FILE_TYPE_TTY, 
    FILE_DIR,
    FILE_NORMAL,
}file_type_t;

/**
 * @brief 文件结构体
 * @param file_name 文件名
 * @param type 文件类型
 * @param size 文件大小
 * @param ref 引用计数
 * @param dev_id 设备号
 * @param pos 当前读写位置
 * @param mode 打开模式,只读、只写、读写
 * @param fs 指向对应文件系统的指针
 * @param p_index 文件在文件目录中的索引
 * @param sblk 文件起始块号
 * @param cblk 文件当前读取的块号
 */
typedef struct _file_t{
    char file_name[FILE_NAME_SIZE];
    file_type_t type; 
    uint32_t size; 
    int ref; 
    int dev_id; 
    int pos; 
    int mode;
    struct _fs_t* fs;
    int p_index;
    int sblk;
    int cblk;
}file_t;

file_t* file_alloc(void);
void file_free(file_t* file);
void file_table_init(void);
void file_inc_ref(file_t* file);
#endif