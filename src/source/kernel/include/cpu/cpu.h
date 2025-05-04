#ifndef CPU_H
#define CPU_H

#include "comm/types.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"

#pragma pack(1)
/**
 * @brief gdt表项结构体
 * @param limit15_0 段界限的低16位
 * @param base15_0 段基址的低16位
 * @param base23_16 段基址的16位到23位
 * @param attr 段界限+段属性 其布局为 8bit段属性+4bit段界限+4bit段属性 （顺序由低到高）
 * @param base31_24 段基址24到31位
 */
typedef struct _segment_desc_t{
    uint16_t limit15_0;  
    uint16_t base15_0;  
    uint8_t base23_16; 
    uint16_t attr;    
    uint8_t base31_24; 
}segment_desc_t;

/**
 * @brief idt表项结构体
 * @param offset15_0 中断处理函数地址的低16位
 * @param selector 中断处理函数的段选择子
 * @param attr 中断处理函数的属性
 * @param offset31_16 中断处理函数地址的高16位
 */
typedef struct _gate_desc_t{
    uint16_t offset15_0;
    uint16_t selector;
    uint16_t attr;
    uint16_t offset31_16;
}gate_desc_t;

/// @brief 定义idt的attr的P位，指示该idt表项是否存在
#define GATE_P_PRESENT (1 << 15)

/// @brief 定义idt的attr中设置DPL权限位，权限为0
#define GATE_DPL0 (0 << 13)

/// @brief 定义idt的attr中设置DPL权限位，权限为3
#define GATE_DPL3 (3 << 13)

// 设置为32位中断
#define GATE_TYPE_INT (0xE << 8)
#define GATE_TYPE_SYSCALL  (0xC << 8)

// 定义gdt的attr位
// 注意位是从0开始！！！！

/**
 * @brief 段描述符属性 G (Granularity) 标志位
 *
 * @details
 * G标志位决定段界限(Limit)的粒度单位：
 * - G=0：Limit单位为字节(byte)，段长度范围为1B-1MB
 * - G=1：Limit单位为4KB，段长度范围为4KB-4GB
 *
 * @note 当G=0时，20位的Limit值前面补3个0
 * @note 当G=1时，20位的Limit值后面补FFF（乘以4KB）
 *
 * @note
 * 如果Limit拼接后为FFFFF：
 * G=0 则实际范围为 000FFFFF (1MB)
 * G=1 则实际范围为 FFFFFFFF (4GB)
 */
#define SEG_G (1 << 15) 

// D/B标志位D = 1采用32位寻址方式,D = 0采用16位寻址方式。
#define SEG_D (1 << 14)

// L标志位用于64位位于第13位 以及AVL标志位位于第12位。这两位一般都为0
// 段描述符的P标志位，指示该描述符是否存在
#define SEG_P_PRESENT (1 << 7) 

/// @brief 设置其特权0标志位，位于第6位和第7位 DPL即描述符特权级别
#define SEG_DPL0 (0 << 5)
 
/// @brief 设置特权3标志位
#define SEG_DPL3 (0x3 << 5)

#define SEG_CPL0 (0 << 0)
#define SEG_CPL3 (3 << 0)

/// @brief 设置S标志位，S = 1代码段或者数据段描述符，S = 0系统段描述符。位于第5位
#define SEG_S_SYSTEM (0 << 4)

/// @brief 设置S标志位，S = 1代码段或者数据段描述符，S = 0系统段描述符。位于第5位
#define SEG_S_NORMAL (1 << 4)

/// @brief 低四位共同组成type标志位，TYPE标志位决定了段的类型，这是代码段
#define SEG_TYPE_CODE (1 << 3)

/// @brief 低四位共同组成type标志位，TYPE标志位决定了段的类型，这是数据段
#define SEG_TYPE_DATA (0 << 3)

/// @brief 决定其是否为可读写
#define SEG_TYPE_RW (1 << 1)

// 这里设置的TSS对应的type
#define SEG_TYPE_TSS (9<<0)

/// @brief 设置EFLAGS的标志位，该位默认为1
#define EFLAGS_DEFAULT (1<<1)

/// @brief 开中断
#define EFLAGS_IF      (1<<9)

void cpu_init(void);
void segment_desc_set(int selector,uint32_t base,uint32_t limit,uint16_t attr);
void gate_desc_set(gate_desc_t* desc,uint16_t selector,uint32_t offset,uint16_t attr);
int gdt_alloc_desc();
void gdt_free_sel(int tss_sel);
typedef struct _tss_t{
    uint32_t pre_link;
    uint32_t esp0,ss0,esp1,ss1,esp2,ss2;
    uint32_t cr3; // 使用的页表
    uint32_t eip,eflags,eax,ecx,edx,ebx,esp,ebp,esi,edi;
    uint32_t es,cs,ss,ds,fs,gs;
    uint32_t ldt;
    uint32_t iomap; // IO位图
}tss_t;
// 切换对应tss

#pragma pack()
void switch_to_tss(int tss_sel);
#endif