#ifndef TTY_H
#define TTY_H

#include "dev/dev.h"
#include "ipc/sem.h"

// tty设备的缓存大小
#define TTY_OBUF_SIZE  512
#define TTY_IBUF_SIZE  512

#define TTY_OCRLF      (1<<0) // 换行时输出\r\n

#define TTY_INCLR      (1<<0) 
#define TTY_IECHO      (1<<1) // 输入时回显

/// @brief 终端设备的个数
#define TTY_NR  8

/**
 * @brief tty设备的FIFO队列结构体
 * @param buf 存储FIFO队列的数据区
 * @param size FIFO队列的大小
 * @param read 读指针
 * @param write 写指针
 * @param count FIFO队列中数据的个数
 */
typedef struct _tty_fifo_t{
    char* buf;
    int size;

    // 读指针，写指针
    int read,write;

    int count;
}tty_fifo_t;

/** 
 * @brief tty设备结构体
 * @param obuf 输出缓存区
 * @param ibuf 输入缓存区
 * @param ofifo 输出FIFO队列，用于console显示
 * @param ififo 输入FIFO队列，用于接收键盘的输入
 * @param console_index 对应的console的索引号
 * @param osem 输出信号量
 * @param isem 输入信号量
 * @param iflags 输入标志
 * @param oflags 输出标志
*/
typedef struct _tty_t{
    char obuf[TTY_OBUF_SIZE];
    char ibuf[TTY_IBUF_SIZE];

    tty_fifo_t ofifo;
    tty_fifo_t ififo;

    int console_index;

    sem_t osem;

    sem_t isem;

    int iflags; 
    int oflags; 
}tty_t;

int tty_open(device_t* dev);
int tty_read(device_t* dev,int addr,char* buf,int size);
int tty_write(device_t* dev,int addr,char* buf,int size);
int tty_control(device_t*dev,int cmd,int arg0,int arg1);
void tty_close(device_t* dev);

void tty_fifo_init(tty_fifo_t* fifo,char* buf,int size);
int tty_fifo_put(tty_fifo_t* fifo,char c);
int tty_fifo_get(tty_fifo_t* fifo,char* c);

void tty_select(int tty);
void tty_in(char ch);
#endif