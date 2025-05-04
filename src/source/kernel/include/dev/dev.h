#ifndef DEV_H
#define DEV_H

#define DEV_NAME_SIZE       32

/// @brief 定义设备类型
enum{
    DEV_UNKNOWN=0,
    DEV_TTY
};

struct _dev_desc_t;

/**
 * @brief 设备结构体
 * @param desc 设备类型以及对应的操作函数的指针
 * @param mode 设备状态
 * @param minor 从设备号
 * @param data 暂存参数
 * @param open_count 被几个应用打开
 */
typedef struct _device_t{
    struct _dev_desc_t* desc;

    int mode;

    int minor;

    void* data;

    int open_count;
}device_t;

/**
 * @brief 设备描述符结构体
 * @param name 设备名称
 * @param major 主设备号，指明设备类型
 * @param open 打开设备的函数指针
 * @param read 读设备的函数指针
 * @param write 写设备的函数指针
 * @param control 控制设备的函数指针
 * @param close 关闭设备的函数指针
 */
typedef struct _dev_desc_t{

    char name[DEV_NAME_SIZE];
    int major;

    int (*open)(device_t* dev);
    int (*read)(device_t* dev,int addr,char* buf,int size);
    int (*write)(device_t* dev,int addr,char* buf,int size);
    int (*control)(device_t* dev,int cmd,int arg0,int arg1);
    void (*close)(device_t* dev);
}dev_desc_t;


int dev_open(int major,int minor,void* data);
int dev_read(int dev_id,int addr,char* buf,int size);
int dev_write(int dev_id,int addr,char* buf,int size);
int dev_control(int dev_id,int cmd,int arg0,int arg1);
void dev_close(int dev_id);

#endif