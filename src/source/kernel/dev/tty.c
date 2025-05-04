#include "dev/tty.h"
#include "tools/log.h"
#include "dev/kbd.h"
#include "dev/console.h"
#include "cpu/irq.h"

static tty_t tty_devs[TTY_NR];

/// @brief 当前使用的tty设备的索引
static int curr_tty=0;

static tty_t* get_tty(device_t* dev){
    int idx=dev->minor;
    if((idx<0) || (idx>=TTY_NR) || (!dev->open_count)){
        log_printf("tty is not opened. tty=%d\n",idx);
        return (tty_t*)0;
    }

    return tty_devs+idx;
}

void tty_fifo_init(tty_fifo_t* fifo,char* buf,int size){
    fifo->buf=buf;
    fifo->size=size;
    fifo->count=0;
    fifo->read=fifo->write=0;
}

/**
 * @brief 向tty的输出缓存中写入一个字符
 * @param fifo tty的输出缓存
 * @param c 要写入的字符
 * @return 0 成功，-1 失败
 */
int tty_fifo_put(tty_fifo_t* fifo,char c){
    irq_state_t state=irq_enter_protection();
    if(fifo->count>=fifo->size){

        irq_leave_protection(state);
        return -1;
    }

    fifo->buf[fifo->write++]=c;
    if(fifo->write>=fifo->size){
        fifo->write=0;
    }
    
    fifo->count++;

    return 0;
}

int tty_fifo_get(tty_fifo_t* fifo,char* c){
    irq_state_t state=irq_enter_protection();
    if(fifo->count<=0){
        irq_leave_protection(state);
        return -1;
    }

    *c=fifo->buf[fifo->read++];
    if(fifo->read>=fifo->size){
        fifo->read=0;
    }
    fifo->count--;

    irq_leave_protection(state);
    return 0;
}

int tty_open(device_t* dev){
    int idx=dev->minor;
    if((idx<0)&&(idx>=TTY_NR)){
        log_printf("open tty failed. incorrect tty num=%d\n",idx);
        return -1;
    }

    tty_t* tty=tty_devs+idx;

    tty_fifo_init(&tty->ofifo,tty->obuf,TTY_OBUF_SIZE);
    sem_init(&tty->osem,TTY_OBUF_SIZE);
    tty_fifo_init(&tty->ififo,tty->ibuf,TTY_IBUF_SIZE);
    sem_init(&tty->isem,0);

    tty->oflags=TTY_OCRLF;
    tty->console_index=idx;

    tty->iflags=TTY_IECHO | TTY_INCLR;

    kbd_init();    
    console_init(tty->console_index);
    
    return 0;
}

/**
 * @brief tty设备的读函数
 * @param dev 获取device_t结构体从中取出minor根据minor选择tty设备
 * @param addr 读的起始地址，暂时不使用
 * @param buf 读到的字符存放的地址
 * @param size 读的字符的个数
 * @return 读到的字符的个数，失败返回-1
 */
int tty_read(device_t* dev,int addr,char* buf,int size){
    if(size < 0){
        return -1;
    }

    tty_t* tty=get_tty(dev);
    char* pbuf=buf;
    int len=0;
    while(len < size){
        sem_wait(&tty->isem);

        char ch;
        tty_fifo_get(&tty->ififo,&ch);
        switch(ch){
            
            // 处理推格键
            case 0x7F:
                if(len == 0){
                   continue;
                }

                len--;
                pbuf--;
                break;
            case '\n':
                if((tty->iflags & TTY_INCLR) && (len < size-1)){
                    *pbuf++='\r';
                    len++;
                }
                *pbuf++='\n';
                len++;
                break;
            default:
                *pbuf++=ch;
                len++;
                break;
        }

        // 处理回显
        if(tty->iflags & TTY_IECHO){
            tty_write(dev,0,&ch,1);
        }

        if((ch == '\n') || (ch == '\r')){
            break;
        }
    }
    return len;
}

int tty_write(device_t* dev,int addr,char* buf,int size){
    if (size < 0){
        return -1;
    }

    tty_t* tty=get_tty(dev);
    if (!tty){
        return -1;
    }

    int len =0;
    while(size){
        char c=*buf++;

        if(c == '\n' && (tty->oflags & TTY_OCRLF)){
            sem_wait(&tty->osem);
            int err=tty_fifo_put(&tty->ofifo,'\r');
            if (err<0){
                break;
            }
        }

        sem_wait(&tty->osem);
        int err=tty_fifo_put(&tty->ofifo,c);
        if (err<0){
            break;
        }
        len++;
        size--;

        console_write(tty);
    }

    return len;
}

int tty_control(device_t*dev,int cmd,int arg0,int arg1){
    return 0;
}

void tty_close(device_t* dev){

}

/**
* @brief 选择curr_tty索引的tty设备，然后将ch放入tty的输入缓存中
* @param ch 要放入tty设备的输入缓存中的字符
*/ 
void tty_in(char ch){
    tty_t* tty=tty_devs+curr_tty;

    if(sem_count(&tty->isem) >= TTY_IBUF_SIZE){
        return;
    }

    tty_fifo_put(&tty->ififo,ch);
    sem_notify(&tty->isem);

}

/**
 * @brief 选择tty设备
 * @param tty 要切换的tty设备的索引号
*/
void tty_select(int tty){
    if(tty!=curr_tty){
        console_select(tty);
        curr_tty=tty;
    }
}

dev_desc_t dev_tty_desc={
    .name="tty",
    .major=DEV_TTY,
    .open=tty_open,
    .read=tty_read,
    .write=tty_write,
    .control=tty_control,
    .close=tty_close,
};