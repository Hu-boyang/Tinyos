#ifndef LOADER_H
#define LOADER_H

#include "comm/types.h"
#include "comm/boot_info.h"
#include "comm/cpu_instr.h"
#include "comm/elf.h"
// 该函数位于Start.S中
void protect_mode_entry(void);

typedef struct SMAP_entry {
    uint32_t BaseL;
    uint32_t BaseH;
    uint32_t LengthL; 
    uint32_t LengthH;
    uint32_t Type; 
    uint32_t ACPI;
}__attribute__((packed)) SMAP_entry_t;

// boot_info_t 位于comm/boot_info.h中
// boot_info_t 只存储ram基地址以及长度
extern boot_info_t boot_info;
#endif // LOADER_H
