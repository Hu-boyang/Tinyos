	.code16
 	.text
 	.extern loader_entry
	.global _start
_start:
	// 跳转到loader_16.c的loader_entry
	jmp loader_entry
	.code32
	.text
	.global protect_mode_entry
	.extern load_kernel
protect_mode_entry:
	mov $16,%ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	// 跳转加载内核的函数
	jmp $8,$load_kernel

