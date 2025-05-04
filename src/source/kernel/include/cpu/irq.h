#ifndef IRQ_H
#define IRQ_H
#include "comm/types.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "os_cfg.h"
#include "tools/log.h"

// 设置中断号码
#define IRQ0_DE             0
#define IRQ1_DB             1
#define IRQ2_NMI            2
#define IRQ3_BP             3
#define IRQ4_OF             4
#define IRQ5_BR             5
#define IRQ6_UD             6
#define IRQ7_NM             7
#define IRQ8_DF             8
#define IRQ10_TS            10
#define IRQ11_NP            11
#define IRQ12_SS            12
#define IRQ13_GP            13
#define IRQ14_PF            14
#define IRQ16_MF            16
#define IRQ17_AC            17
#define IRQ18_MC            18
#define IRQ19_XM            19
#define IRQ20_VE            20
#define IRQ21_CP            21
#define IRQ0_TIMER          0x20
// 设置8259A相关参数
#define PIC0_ICW1			0x20
#define PIC0_ICW2			0x21
#define PIC0_ICW3			0x21
#define PIC0_ICW4			0x21
#define PIC0_OCW2			0x20
#define PIC0_IMR			0x21

#define PIC1_ICW1			0xa0
#define PIC1_ICW2			0xa1
#define PIC1_ICW3			0xa1
#define PIC1_ICW4			0xa1
#define PIC1_OCW2			0xa0
#define PIC1_IMR			0xa1

#define PIC_ICW1_ICW4		(1 << 0)		
#define PIC_ICW1_ALWAYS_1	(1 << 4)		
#define PIC_ICW4_8086	    (1 << 0)       

#define PIC_OCW2_EOI		(1 << 5)		

#define IRQ_PIC_START		0x20		

#define IRQ1_KEYBOARD       0x21

// 定义idt表大小
#define IDT_TABLE_NR 128

#define ERR_PAGE_P          (1 << 0)
#define ERR_PAGE_WR         (1 << 1)
#define ERR_PAGE_US         (1 << 1)

#define ERR_EXT             (1 << 0)
#define ERR_IDT             (1 << 1)

/// @brief 存储中断前各寄存器状态值
typedef struct _exception_frame_t{
    uint32_t gs,fs,es,ds;
    uint32_t edi,esi,ebp,esp,ebx,edx,ecx,eax;
    uint32_t num,error_code;
    uint32_t eip,cs,eflags;
    uint32_t esp3,ss3;
} exception_frame_t;

typedef void (*irq_handler_t)(exception_frame_t* frame);


void irq_init(void);

// 添加对应中断号的处理函数
int irq_install(int irq_num,irq_handler_t handler);

// 处理未设置的具体中断函数的中断
// 在init/Start.S中
void exception_handler_unknown(void);

/// @brief 设置除0异常处理函数
void exception_handler_divider(void);

// 设置各种异常函数
void exception_handler_Debug (void);
void exception_handler_NMI (void);
void exception_handler_breakpoint (void);
void exception_handler_overflow (void);
void exception_handler_bound_range (void);
void exception_handler_invalid_opcode (void);
void exception_handler_device_unavailable (void);
void exception_handler_double_fault (void);
void exception_handler_invalid_tss (void);
void exception_handler_segment_not_present (void);
void exception_handler_stack_segment_fault (void);
void exception_handler_general_protection (void);
void exception_handler_page_fault (void);
void exception_handler_fpu_error (void);
void exception_handler_alignment_check (void);
void exception_handler_machine_check (void);
void exception_handler_smd_exception (void);
void exception_handler_virtual_exception (void);

void irq_enable(int irq_num);
void irq_disable(int irq_num);
void irq_enable_global(void);
void irq_disable_global(void);
void pic_send_eoi(int irq_num);

/// @brief 判断是否进入临界区的状态值
typedef uint32_t irq_state_t;

irq_state_t irq_enter_protection(void);
void irq_leave_protection(irq_state_t state);
#endif