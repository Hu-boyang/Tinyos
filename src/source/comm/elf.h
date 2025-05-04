#ifndef ELF_H
#define ELF_H
#include "types.h"
/// @brief Elf32_Addr 指明运行地址
typedef uint32_t Elf32_Addr; 
typedef uint16_t Elf32_Half;
/// @brief Elf32_Off 指明文件偏移量
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Word;

/*
    #pragma pack(1)是一个编译器指令，用于控制结构体的内存对齐。
    默认情况下，编译器会根据结构体中最大成员的大小来对齐整个结构体，这可能会导致内存空间的浪费。
    使用#pragma pack(1)可以将结构体的对齐方式设置为1字节，从而减少内存占用。
*/
#pragma pack(1)

#define EI_NIDENT       16
#define ELF_MAGIC       0x7F

/* 
    Elf32_Ehdr 的全称是 ELF 32-bit Header，
    即 32 位 ELF 文件头。它是 ELF 文件格式的第一个结构体，
    用于描述整个 ELF 文件的基本信息，
    提供了该文件的结构、类型、入口点等关键信息。
    Elf32_Ehdr 结构体位于 ELF 文件的开始部分，
    其他的所有内容（如程序头、节头等）都依赖于它来定位。
*/
/**
 * @brief ELF文件头结构体
 * @param e_ident 指明elf字符信息
 * @param e_type 指明elf目标文件类型
 * @param e_machine 指明在哪种硬件平台上运行
 * @param e_version 指明版本信息
 * @param e_entry 指明操作系统运行该程序，将控制权装交的地址
 * @param e_phoff 用来指明程序头表在文件中的字节偏移量
 * @param e_shoff 用来指明节头表在文件中的字节偏移量
 * @param e_flags 用指明与处理器相关的标志
 * @param e_ehsize 用来指明elf header的字节大小
 * @param e_phentsize 用来指明程序头表中每个条目的大小，即Elf32_Phdr
 * @param e_phnum 程序头表段的数量，就是段的数量
 * @param e_shentsize 用来指明节头表每个条目大小
 * @param e_shnum 用来指明节的个数
 * @param e_shstrndx 这个先不用管
 */
typedef struct {
    char e_ident[EI_NIDENT]; 
    Elf32_Half e_type; 
    Elf32_Half e_machine; 
    Elf32_Word e_version; 
    Elf32_Addr e_entry; 
    Elf32_Off e_phoff; 
    Elf32_Off e_shoff; 
    Elf32_Word e_flags;
    Elf32_Half e_ehsize; 
    Elf32_Half e_phentsize; 
    Elf32_Half e_phnum; 
    Elf32_Half e_shentsize; 
    Elf32_Half e_shnum; 
    Elf32_Half e_shstrndx; 
}Elf32_Ehdr;

#define PT_LOAD         1

/*
    Elf32_Phdr 的全称是 ELF 32-bit Program Header，
    即 32 位 ELF 程序头。
    它是 ELF 文件中的一个结构体，
    用于描述文件中的各个程序段（segment）以及如何将它们加载到内存中。
*/

/**
 * @brief ELF程序头结构体
 * @param p_type 用来指明该段的类型
 * @param p_offset 用来指明该段在文件内的偏移地址
 * @param p_vaddr 用来指明本段在内存中的起始地址
 * @param p_paddr 指明要拷贝的物理内存位置
 * @param p_filesz 用来指明本段在文件中的大小
 * @param p_memsz 用来指明本段在内存中大小
 * @param p_flags 用来指明本段相关标志
 * @param p_align 用来指明本段在文件和内存对齐方式，p_align应该是2的幂次
 */
typedef struct {
    Elf32_Word p_type; 
    Elf32_Off p_offset; 
    Elf32_Addr p_vaddr;
    Elf32_Addr p_paddr;
    Elf32_Word p_filesz; 
    Elf32_Word p_memsz; 
    Elf32_Word p_flags; 
    Elf32_Word p_align;
} Elf32_Phdr;

#pragma pack()
#endif