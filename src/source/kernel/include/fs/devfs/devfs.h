#ifndef DEVFS_H
#define DEVFS_H

#include "fs/fs.h"

/**
 * @brief 文件系统类型结构体
 * @param dev_type 支持的设备类型
 * @param file_type 支持的文件类型
 */
typedef struct _devfs_type_t{
    const char* name;
    int dev_type;
    int file_type;
}devfs_type_t;

int devfs_mount(struct _fs_t* fs,int major,int minor);
void devfs_unmount(struct _fs_t* fs);
int devfs_open(struct _fs_t* fs,const char* path,file_t* file);
int devfs_read(char* buf,int size,file_t* file);
int devfs_write(char*buf,int size,file_t* file);
void devfs_close(file_t* file);
int devfs_seek(file_t* file,uint32_t offset,int dir);
int devfs_stat(file_t*file,struct stat* st);

#endif