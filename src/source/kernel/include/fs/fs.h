#ifndef FS_H
#define FS_H

#include <sys/stat.h>

#include "fs/file.h"
#include "tools/list.h"
#include "ipc/mutex.h"
#include "fs/fatfs/fatfs.h"
#include "applib/lib_syscall.h"

struct _fs_t;

/**
 * @brief 文件系统操作函数表
 * @param mount 挂载文件系统
 * @param unmount 卸载文件系统
 * @param open 打开文件
 * @param read 读文件
 * @param write 写文件
 * @param close 关闭文件
 * @param seek 定位文件指针
 * @param stat 获取文件状态
 * @param opendir 打开目录
 * @param readdir 读取目录项
 * @param closedir 关闭目录
 * @param ioctl 控制文件操作
 * @param unlink 删除文件
 */
typedef struct _fs_op_t{
    int (*mount)(struct _fs_t* fs,int major,int minor);
    void (*unmount)(struct _fs_t* fs);
    int (*open)(struct _fs_t* fs,const char* path,file_t* file);
    int (*read)(char* buf,int size,file_t* file);
    int (*write)(char* buf,int size,file_t* file);
    void (*close)(file_t* file);
    int (*seek)(file_t* file,uint32_t offset,int dir);

    // stat结构体在<sys/stat.h>中定义
    int (*stat)(file_t* file,struct stat* st);

    int (*opendir)(struct _fs_t *fs,const char *name,DIR *dir);
    int (*readdir)(struct _fs_t *fs,DIR *dir,struct dirent *dirent);
    int (*closedir)(struct _fs_t *fs,DIR *dir);

    int (*ioctl)(file_t* file,int cmd,int arg0,int arg1);
    int (*unlink)(struct _fs_t *fs,const char *path);
}fs_op_t;

/// @brief 文件挂载点名称的大小
#define FS_MOUNT_SIZE 512

/**
 * @brief 文件系统类型
 * @param FS_DEVFS 设备文件系统
 * @param FS_FAT16 FAT16文件系统
 */
typedef enum _fs_type_t{
    FS_DEVFS,
    FS_FAT16,
}fs_type_t;

/**
 * @brief 文件操作结构体
 * @param op 文件操作函数表的指针
 * @param mount_point 挂载点,这是一个字符类型的字符串,表明其属于哪种文件系统
 * @param type 文件系统类型
 * @param data 文件系统需要保存的数据
 * @param dev_id 设备号,用来识别磁盘分区
 * @param node 链表节点
 * @param mutex 互斥锁
 * @param fat_data FAT文件系统数据,如果是FAT文件系统,则包含FAT相关的数据
 */
typedef struct _fs_t{
    char mount_point[FS_MOUNT_SIZE];
    fs_type_t type;

    fs_op_t* op;
    void* data;
    int dev_id;
    list_node_t node;
    mutex_t* mutex;

    union{
        fat_t fat_data;
    };
    
}fs_t;

void fs_init(void);

int sys_open(const char* name,int flags,...);
int sys_read(int file,char* ptr,int len);
int sys_write(int file,char* ptr,int len);
int sys_lseek(int file,int ptr,int dir);
int sys_close(int file);

int sys_isatty(int file);
int sys_fstat(int file,struct stat* st);

int sys_dup(int file);

int path_to_num(const char* path,int* num);
const char* path_next_child(const char* path);

int sys_opendir(const char* path, DIR* dir);
int sys_readdir(DIR* dir, struct dirent* dirent);
int sys_closedir(DIR* dir);

int sys_ioctl(int fd,int cmd,int arg0,int arg1);
int sys_unlink(const char* path);
#endif