#include "dev/disk.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "comm/cpu_instr.h"
#include "comm/boot_info.h"
#include "dev/dev.h"


static mutex_t disk_mutex;
static sem_t op_sem;

/// @brief 有进程引起磁盘中断的标志位
static int task_on_op=0;

/// @brief 磁盘信息的表
static disk_t disk_buf[DISK_CNT];

/**
 * @brief 向磁盘发送命令
 * @param disk 磁盘结构体指针
 * @param start_sector 起始扇区号
 * @param sector_count 扇区数
 * @param cmd 命令
 */
static void disk_send_cmd(disk_t* disk,uint32_t start_sector,uint32_t sector_count,int cmd){
    outb(DISK_DRIVE(disk), DISK_DRIVE_BASE | disk->drive);		// 使用LBA寻址，并设置驱动器

	// 必须先写高字节
	outb(DISK_SECTOR_COUNT(disk), (uint8_t) (sector_count >> 8));	// 扇区数高8位
	outb(DISK_LBA_LO(disk), (uint8_t) (start_sector >> 24));		// LBA参数的24~31位
	outb(DISK_LBA_MID(disk), 0);									// 高于32位不支持
	outb(DISK_LBA_HI(disk), 0);										// 高于32位不支持
	outb(DISK_SECTOR_COUNT(disk), (uint8_t) (sector_count));		// 扇区数量低8位
	outb(DISK_LBA_LO(disk), (uint8_t) (start_sector >> 0));			// LBA参数的0-7
	outb(DISK_LBA_MID(disk), (uint8_t) (start_sector >> 8));		// LBA参数的8-15位
	outb(DISK_LBA_HI(disk), (uint8_t) (start_sector >> 16));		// LBA参数的16-23位

	// 选择对应的主-从磁盘
	outb(DISK_CMD(disk), (uint8_t)cmd);
}

/**
 * @brief 从磁盘中读数据
 * @param disk 磁盘结构体指针
 * @param buf 缓冲区指针
 */
static inline void disk_read_data(disk_t* disk, void* buf,int size){
    uint16_t * c = (uint16_t *)buf;
    for (int i = 0; i < size / 2; i++) {
        *c++ = inw(DISK_DATA(disk));
    }
}

/**
 * @brief 向磁盘中写数据
 * @param disk 磁盘结构体指针
 * @param buf 缓冲区指针
 */
static inline void disk_write_data(disk_t*disk,void* buf,int size){
    uint16_t * c = (uint16_t *)buf;
    for (int i = 0; i < size / 2; i++) {
        outw(DISK_DATA(disk), *c++);
    }
}

/**
 * @brief 等待磁盘直到磁盘不是忙的状态返回
 * @param disk 磁盘结构体指针
 * @return 如果磁盘状态为错误则返回-1，否则返回0
 */
static int disk_wait_data(disk_t* disk){
    uint8_t status;
	do {
        // 等待数据或者有错误
        status = inb(DISK_STATUS(disk));

        // 这里的几个宏的位置不能替换要么会发生错误
        if((status & (DISK_STATUS_BUSY | DISK_STATUS_DRQ | DISK_STATUS_ERR)) != DISK_STATUS_BUSY){
                break;
        }   
    }while (1);

    // 检查是否有错误
    return (status & DISK_STATUS_ERR) ? -1 : 0;
}

/**
 * @brief 通过disk_t结构体中的字段打印磁盘信息
 * @param disk 磁盘结构体指针
 */
static void print_disk_info(disk_t* disk){
    log_printf("%s",disk->name);
    log_printf("port base: %x",disk->port_base);
    log_printf("total size: %dMB",(disk->sector_size * disk->sector_count) / 1024 / 1024);

    for(int i=0;i<DISK_PRIMARY_PART_NR;i++){
        partinfo_t* partinfo=disk->partinfo+i;
        if(partinfo->type!=FS_INVALID){
            log_printf("%s:type:%x, start_sector:%d, count:%d",
                partinfo->name,partinfo->type,partinfo->start_sector,partinfo->total_sector
           );
        }
    }
}

static int detect_part_info(disk_t* disk){
    mbr_t mbr;
    
    // 读取磁盘第0个扇区
    disk_send_cmd(disk,0,1,DISK_CMD_READ);

    int err=disk_wait_data(disk);
    
    if(err < 0){
        log_printf("read mbr failed.");
        return err;
    }

    disk_read_data(disk,&mbr,sizeof(mbr));
    part_item_t* item=mbr.part_item;
    partinfo_t* part_info=disk->partinfo+1;

    // 读取mbr_t内容并将其内容写入part_info数组对应的元素
    for(int i=0;i<MBR_PRIMARY_PART_NR;i++,item++,part_info++){
        part_info->type = item->system_id;

        if(part_info->type==FS_INVALID){
            part_info->total_sector=0;
            part_info->start_sector=0;
            part_info->disk=(disk_t*)0;
        }
        else{
            kernel_sprintf(part_info->name,"%s%d",disk->name,i+1);
            part_info->start_sector=item->relative_sectors;
            part_info->total_sector=item->total_sectors;
            part_info->disk=disk;
        }
    }
}

/**
 * @brief 识别磁盘
 * @param disk 磁盘结构体指针
 * @return 如果磁盘存在则返回0，否则返回-1
 */
static int identify_disk(disk_t* disk){
    // 发送识别磁盘命令
    disk_send_cmd(disk,0,0,DISK_CMD_IDENTIFY);

    // 判断磁盘是否存在
    int err=inb(DISK_STATUS(disk));
    if(err == 0){
        log_printf("%s doesn't exist\n",disk->name);
        return -1; // 磁盘不存在
    }

    // 等待磁盘准备好
    err=disk_wait_data(disk);
    if(err < 0){
        log_printf("disk[%s]: read disk failed\n",disk->name);
        return err;
    }

    uint16_t buf[256];
    disk_read_data(disk,buf,sizeof(buf));
    disk->sector_count=*(uint32_t*)(buf + 100);
    disk->sector_size=SECTOR_SIZE;

    // 设置第一个分区表项，将第一个分区设置为FS_INBALID不使用
    partinfo_t* part=disk->partinfo+0;
    part->disk=disk;
    kernel_sprintf(part->name,"%s%d",disk->name,0);
    part->start_sector=0;
    part->total_sector=disk->sector_count;
    part->type=FS_INVALID;

    // 读取mbr中分区表的信息设置别的分区partinfo_t结构体
    detect_part_info(disk);

    return 0;

}

/**
 * @brief 磁盘初始化函数  
 */ 
void disk_init(void){
    log_printf("Check disk...\n");

    mutex_init(&disk_mutex);
    sem_init(&op_sem,0);

    kernel_memset(disk_buf,0,sizeof(disk_buf));
    for(int i=0;i<DISK_CNT;i++){
        disk_t* disk=disk_buf+i;

        // 磁盘以sda表示第一块儿,sdb表示第二块儿
        kernel_sprintf(disk->name,"sd%c",i+'a');
        disk->drive=(i==0)?DISK_MASTER:DISK_SLAVE;
        disk->port_base=IOBASE_PRIMARY;

        disk->mutex=&disk_mutex;
        disk->op_sem=&op_sem;

        int err=identify_disk(disk);
        if(err == 0){
            print_disk_info(disk);
        }


    }
}

/**
 * @brief 打开磁盘设备
 * @param dev 设备结构体指针
 * @return 如果打开成功则返回0，否则返回-1
 */
int disk_open(device_t* dev){
    int disk_idx=(dev->minor >> 4) - 0xa;
    int part_idx= dev->minor & 0xf;

    if((disk_idx >= DISK_CNT) || (part_idx >= DISK_PRIMARY_PART_NR)){
        log_printf("device minor error: %d\n",dev->minor);
        return -1;
    }

    disk_t* disk=disk_buf + disk_idx;
    if(disk->sector_count == 0){
        log_printf("disk not exist, dev:sd%x",dev->minor);
        return -1;
    }

    partinfo_t* part_info=disk->partinfo + part_idx;
    if(part_info->total_sector == 0){
        log_printf("part not exist, dev:sd%x",dev->minor);
        return -1;
    }

    // 将分区信息直接保存在data里面这样以后直接从分区中读取相关信息
    dev->data=part_info;

    irq_install(IRQ14_HARDDISK_PRIMARY,(irq_handler_t)exception_handler_ide_primary);
    irq_enable(IRQ14_HARDDISK_PRIMARY);

    return 0;
}

int disk_read(device_t* dev,int addr,char* buf,int size){
    partinfo_t* partinfo=(partinfo_t*)dev->data;
    if(partinfo == (partinfo_t*)0){
        log_printf("Get part info failed. device: %d",dev->minor);;
        return -1;
    }

    disk_t* disk=partinfo->disk;
    if(disk == (disk_t*)0){
        log_printf("No disk. device: %d",dev->minor);
        return -1;
    }

    mutex_lock(disk->mutex);

    task_on_op=1;

    disk_send_cmd(disk,partinfo->start_sector + addr, size, DISK_CMD_READ);
    int cnt;
    for(cnt=0;cnt<size;cnt++,buf+=disk->sector_size){
        if(task_current()){
            sem_wait(disk->op_sem);
        }

        int err=disk_wait_data(disk);
        if(err < 0){
            log_printf("disk(%s) read error, start sector: %d,count: %d",
                disk->name,addr,size
            );
            break;
        }

        disk_read_data(disk,buf,disk->sector_size);

    }

    mutex_unlock(disk->mutex);

    return cnt;
}

int disk_write(device_t* dev,int addr,char* buf,int size){
    partinfo_t* partinfo=(partinfo_t*)dev->data;
    if(partinfo == (partinfo_t*)0){
        log_printf("Get part info failed. device: %d",dev->minor);;
        return -1;
    }

    disk_t* disk=partinfo->disk;
    if(disk == (disk_t*)0){
        log_printf("No disk. device: %d",dev->minor);
        return -1;
    }

    mutex_lock(disk->mutex);

    task_on_op=1;

    disk_send_cmd(disk,partinfo->start_sector + addr, size, DISK_CMD_WRITE);

    int cnt;
    for(cnt=0;cnt<size;cnt++,buf+=disk->sector_size){
        disk_write_data(disk,buf,disk->sector_size);

        if(task_current()){
            sem_wait(disk->op_sem);
        }

        int err=disk_wait_data(disk);
        if(err < 0){
            log_printf("disk(%s) read error, start sector: %d,count: %d",
                disk->name,addr,size
            );
            break;
        }

    }

    mutex_unlock(disk->mutex);

    return cnt;
}

int disk_control(device_t* dev,int cmd,int arg0,int arg1){
    return -1;
}

void disk_close(device_t* dev){

}

void do_handler_ide_primary(exception_frame_t* frame){
    pic_send_eoi(IRQ14_HARDDISK_PRIMARY);

    if(task_on_op && task_current()){
       sem_notify(&op_sem); 
    }
}

dev_desc_t dev_disk_desc={
    .name="disk",
    .major=DEV_DISK,
    .open=disk_open,
    .read=disk_read,
    .write=disk_write,
    .control=disk_control,
    .close=disk_close,
};