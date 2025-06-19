#include "os_cfg.h"
    .text
    .extern kernel_init,init_main
    .global _start
_start:
    // 从栈中取出参数,参数保存在%eax中,最终传递给kernel_init函数
    // 参数位于load_32.c中
    // 以下三步可以简写为: mov 4(%esp),%eax
    // 传递的是boot_info
    push %ebp
    mov %esp,%ebp
    mov 0x8(%ebp),%eax
    
    push %eax

    // kernel_init 位于init.c中
    call kernel_init

    // 加载到对应选择子以及偏移量
    jmp $KERNEL_SELECTOR_CS,$gdt_reload
gdt_reload:
    // 初始化寄存器
    mov $KERNEL_SELECTOR_DS,%ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
    mov $(stack+KERNEL_STACK_SIZE),%esp
    jmp init_main
    
    // .comm symbol, size, alignment
    // symbol：这是你要声明的符号名称，通常是一个变量名，表示该符号在程序中全局可见。
    // size：分配给该符号的内存大小，通常是字节数。
    // alignment：符号的对齐方式，通常是 2 的幂。
    // 例如，alignment = 4 表示符号需要以 4 字节对齐。
    .bss
.comm stack,KERNEL_STACK_SIZE

// 定义宏函数类似c++的泛型
// 用\可以宏替换，例如下面的\name就是宏替换
// num是错误号 with_error_code是错误码
.macro exception_handler name num with_error_code
    .text
    .extern do_handler_\name
    .global exception_handler_\name
exception_handler_\name:

    // with_error_code若为0则压入0即可，若不为0系统自动压入错误码
    .if \with_error_code==0
        push $0
    .endif

    // 压入错误号
    push $\num

    // 对所有寄存器压栈
    pusha
    push %ds
    push %es
    push %fs
    push %gs

    // 这里压入esp相当于压入一个指针该指针指向存储gs寄存器的内存地址
    push %esp

    call do_handler_\name

    // 弹出压入的指针
    add $4,%esp
    
    // 出栈所有寄存器
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa 
    
    // 相当于弹出\name以及\num
    add $(2*4),%esp

    // 中断返回指令特殊返回指令
    iret

// 宏的结束
.endm

exception_handler unknown,-1,0
exception_handler divider,0,0
exception_handler Debug, 1, 0
exception_handler NMI, 2, 0
exception_handler breakpoint, 3, 0
exception_handler overflow, 4, 0
exception_handler bound_range, 5, 0
exception_handler invalid_opcode, 6, 0
exception_handler device_unavailable, 7, 0
exception_handler double_fault, 8, 1
exception_handler invalid_tss, 10, 1
exception_handler segment_not_present, 11, 1
exception_handler stack_segment_fault, 12, 1
exception_handler general_protection, 13, 1
exception_handler page_fault, 14, 1
exception_handler fpu_error, 16, 0
exception_handler alignment_check, 17, 1
exception_handler machine_check, 18, 0
exception_handler smd_exception, 19, 0
exception_handler virtual_exception, 20, 0
exception_handler time,0x20,0
exception_handler kbd,0x21,0
exception_handler ide_primary,0x2e,0

    .global exception_handler_syscall
    .extern do_handler_syscall
exception_handler_syscall:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    pushf

    mov %esp,%eax
    push %eax

    call do_handler_syscall
    add $4,%esp
    
    popf
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    retf $(5*4)