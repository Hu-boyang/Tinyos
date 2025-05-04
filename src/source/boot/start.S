#include "boot.h"

  	// 16位代码，务必加上
  	.code16
 	.text
	.global _start
	.extern boot_entry
_start:
	mov $0, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov $_start, %esp
read_loader:
	// 一下设置读磁盘中断所需的寄存器
	mov $0x8000, %bx	
	mov $0x2, %cx		
	mov $0x2, %ah		
	mov $64, %al		
	mov $0x0080, %dx	
	// 使用int中断指令读磁盘
	int $0x13
	// 失败重新读
	jc read_loader

	
	jmp boot_entry

	
	jmp .
	
	// 引导结束段
	.section boot_end, "ax"
boot_sig: .byte 0x55, 0xaa
