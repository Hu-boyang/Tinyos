#include "cpu/cpu.h"
#include "ipc/mutex.h"
#include "core/syscall.h"
#include "cpu/irq.h"

/// @brief gdt表
static segment_desc_t gdt_table[GDT_TABLE_SIZE];

/// @brief 互斥锁
static mutex_t mutex;

/**
 * @brief 设置gdt表项
 * @param selector gdt表的索引
 * @param base 段基址
 * @param limit 段界限
 * @param attr 段属性
 * @return void
*/
void segment_desc_set(int selector,uint32_t base,uint32_t limit,uint16_t attr){
    segment_desc_t* desc=gdt_table+(selector >> 3);

    // 设置G标志位决定粒度4kb or 1B
    if(limit > 0xFFFFF){
        attr|=0x8000;
        limit/=0x1000;
    }
    desc->limit15_0=limit&0xFFFF;
    desc->base15_0=base&0xFFFF;
    desc->base23_16=(base>>16)&0xFF;
    desc->attr=attr|(((limit>>16)&0xF)<<8);
    desc->base31_24=(base>>24)&0xFF;

}

// 初始化gdt表
void init_gdt(void){
    for(int i=0;i<GDT_TABLE_SIZE;i++){
        segment_desc_set(i<<3,0,0,0);
    }
    segment_desc_set(KERNEL_SELECTOR_DS,0,0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW
        | SEG_D
    );
    segment_desc_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE
        | SEG_TYPE_RW | SEG_D | SEG_G);
    gate_desc_set((gate_desc_t*)(gdt_table+(SELECTOR_SYSCALL >> 3)),
        KERNEL_SELECTOR_CS,(uint32_t)exception_handler_syscall,
        GATE_P_PRESENT | GATE_DPL3 | GATE_TYPE_SYSCALL | SYSCALL_PARAM_COUNT
    );
    lgdt((uint32_t)gdt_table,sizeof(gdt_table));
}

// 设置idt表项
void gate_desc_set(gate_desc_t* desc,uint16_t selector,uint32_t offset,uint16_t attr){
    desc->offset15_0=offset&0xFFFF;
    desc->selector=selector;
    desc->attr=attr;
    desc->offset31_16=(offset>>16)&0xFFFF;
}

/**
 * @brief 初始化mutex锁以及gdt表
 * @return void
*/
void cpu_init(void){
    mutex_init(&mutex);
    init_gdt();
}

/**
 * @brief 分配gdt表项
 * @return 分配成功返回gdt表项的索引，失败返回-1
*/
int gdt_alloc_desc(){
    mutex_lock(&mutex);
    for(int i=1;i<GDT_TABLE_SIZE;i++){
        segment_desc_t* desc=gdt_table+i;
        if(desc->attr==0){
            mutex_unlock(&mutex);
            return i*sizeof(segment_desc_t);
        }
    }
    mutex_unlock(&mutex);
    return -1;
}

void switch_to_tss(int tss_sel){
    far_jump(tss_sel,0);
}

void gdt_free_sel(int sel){
    mutex_lock(&mutex);
    gdt_table[sel/sizeof(segment_desc_set)].attr=0;
    mutex_unlock(&mutex);
}