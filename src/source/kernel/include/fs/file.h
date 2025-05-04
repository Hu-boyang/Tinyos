#ifndef FILE_H
#define FILE_H

#include "comm/types.h"

#define FILE_NAME_SIZE  32 // 文件名的大小
#define FILE_TABLE_SIZE  2048 // 文件表的大小

/**
 * @brief 文件类型
 * @param FILE_UNKNOWN 未知文件类型
 * @param FILE_TYPE_TTY tty设备文件
 */
typedef enum _file_type_t{
    FILE_UNKNOWN=0, // 未知文件类型
    FILE_TYPE_TTY, // tty设备文件
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
 */
typedef struct _file_t{
    char file_name[FILE_NAME_SIZE];
    file_type_t type; 
    uint32_t size; 
    int ref; 
    int dev_id; 
    int pos; 
    int mode;
}file_t;

file_t* file_alloc(void);
void file_free(file_t* file);
void file_table_init(void);
#endif