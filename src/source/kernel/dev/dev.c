#include "dev/dev.h"
#include "cpu/irq.h"

#define DEV_TABLE_SIZE         128

extern dev_desc_t dev_tty_desc;

// dev设备注册
static dev_desc_t* dev_desc_tbl[]={
    &dev_tty_desc,
};

static device_t dev_tb[DEV_TABLE_SIZE];

static int is_devid_bad(int dev_id){
    if((dev_id<0) || (dev_id>=DEV_TABLE_SIZE)){
        return 1;
    }

    if(dev_tb[dev_id].desc==(dev_desc_t*)0){
        return 1;
    }

    return 0;
}

/**
 * @brief 打开设备
 * @param major 主设备号
 * @param minor 从设备号
 * @param data 设备传入的参数
 * @return 设备id，失败返回-1
*/
int dev_open(int major,int minor,void* data){
    irq_state_t state=irq_enter_protection();

    device_t* free_dev=(device_t*)0;

    for(int i=0;i<sizeof(dev_tb)/sizeof(dev_tb[0]);i++){
        device_t* dev=dev_tb+i;
        if(dev->open_count == 0){
            free_dev=dev;
        }
        else if((dev->desc->major == major) && (dev->minor == minor)){
            dev->open_count++;
            irq_leave_protection(state);
            return i;
        }
    }

    dev_desc_t* desc=(dev_desc_t*)0;
    for(int i=0;i<sizeof(dev_desc_tbl)/sizeof(dev_desc_tbl[0]);i++){
        if(dev_desc_tbl[i]->major == major){
            desc=dev_desc_tbl[i];
            break;
        }
    }

    if(desc && free_dev){
       free_dev->minor=minor;
       free_dev->desc=desc;
       free_dev->data=data;

       int err=desc->open(free_dev);
       if(err==0){
           free_dev->open_count++;
           irq_leave_protection(state);

           // dev_tb是起始位置，free_dev是当前位置，两者相减得到当前位置的偏移量
           return free_dev-dev_tb;
       }
    }

    irq_leave_protection(state);
    return -1;
}

// dev_id:设备id，addr:从哪里开始读取
int dev_read(int dev_id,int addr,char* buf,int size){
    if(is_devid_bad(dev_id)){
        return -1;
    }

    device_t* dev=dev_tb+dev_id;
    return dev->desc->read(dev,addr,buf,size);
}

int dev_write(int dev_id,int addr,char* buf,int size){
    if(is_devid_bad(dev_id)){
        return -1;
    }

    device_t* dev=dev_tb+dev_id;
    return dev->desc->write(dev,addr,buf,size);
}

int dev_control(int dev_id,int cmd,int arg0,int arg1){
    if(is_devid_bad(dev_id)){
        return -1;
    }

    device_t* dev=dev_tb+dev_id;
    return dev->desc->control(dev,cmd,arg0,arg1);
}

void dev_close(int dev_id){
    if(is_devid_bad(dev_id)){
        return;
    }
    
    device_t* dev=dev_tb+dev_id;

    irq_state_t state=irq_enter_protection();

    if(--dev->open_count == 0){
        dev->desc->close(dev);
        kernel_memset(dev,0,sizeof(device_t));
    }

    irq_leave_protection(state);
    
    return;
}

