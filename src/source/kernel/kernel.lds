/*
     .text 代码段
     .rodata  常量值段，一般是const修饰的变量
     .data 代码段
     .bss bss存储未初始化的变量 
          和代码段的变量都是static只是一个已经初始化一个没有初始化
    其作用是控制代码和数据具体存放位置，
	这里*(.text)等的作用是将多个文件的对应段合并在一起，
	而且例如.text在.data前面那么最终文件中.text在内存中的地址也比.data要低
*/
SECTIONS {
	PROVIDE(kernel_base = .);

	. = 0x10000;

	PROVIDE(s_text = .);
	.text : {
		*(EXCLUDE_FILE(*first_task* *lib_syscall*) .text)
	}
	.rodata : {
		*(EXCLUDE_FILE(*first_task* *lib_syscall*) .rodata)
	}
	PROVIDE(e_text = .);

	. = ALIGN(4096);
	PROVIDE(s_data = .);
	.data : {
		*(EXCLUDE_FILE(*first_task* *lib_syscall*) .data)
	}
	.bss : {
		*(EXCLUDE_FILE(*first_task* *lib_syscall*) .bss)
	}
	e_data = .;
	. = 0x80000000;
	PROVIDE(s_first_task = LOADADDR(.first_task));
	.first_task : AT(e_data) {
		*first_task_entry*(.text .rodata .bss .data)
		*first_task*(.text .rodata .bss .data)
		*lib_syscall*(.text .rodata .bss .data)
	}
	PROVIDE(e_first_task = LOADADDR(.first_task) + SIZEOF(.first_task));
	PROVIDE(mem_free_start = e_first_task);
}