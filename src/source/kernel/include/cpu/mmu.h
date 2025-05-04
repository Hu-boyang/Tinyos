#ifndef MMU_H
#define MMU_H

#include "comm/types.h"
#include "comm/cpu_instr.h"

/// @brief 该位的设置表示该页目录项是一个4MB的页表项
#define PDE_PS 		(1<<7)

/// @brief cr4寄存器的PSE位，支持二级页表
#define CR4_PSE		(1<<4)

/// @brief cr0寄存器的PG位，支持分页
#define CR0_PG		(1<<31)

/// @brief 页目录项的数量
#define PDE_CNT     1024

/// @brief 页表项的数量
#define PTE_CNT     1024

#define PTE_P       (1 << 0)
#define PDE_P       (1 << 1)
#define PTE_W       (1 << 1)
#define PDE_W       (1 << 1)
#define PDE_U       (1 << 2)
#define PTE_U       (1 << 2)
typedef union _pde_t
{
    uint32_t v;
    struct{
        uint32_t present : 1;
        uint32_t write_enable : 1;
        uint32_t user_mode_acc : 1;
        uint32_t write_through : 1;
        uint32_t cache_disable : 1;
        uint32_t accessed : 1;
        uint32_t : 1;
        uint32_t ps : 1;
        uint32_t : 4;
        uint32_t phy_pt_addr : 20;
    };

}pde_t;

typedef union _pte_t
{
    uint32_t v;
    struct{
        uint32_t present : 1;
        uint32_t write_enable : 1;
        uint32_t user_mode_acc : 1;
        uint32_t write_through : 1;
        uint32_t cache_disable : 1;
        uint32_t accessed : 1;
        uint32_t dirty: 1;
        uint32_t pat : 1;
        uint32_t global : 1;
        uint32_t : 3;
        uint32_t phy_pt_addr : 20;
    };

}pte_t;


/**
 * @brief 将页目录的物理地址写入cr3寄存器
 * @param paddr 页目录表的物理地址 
*/
static inline void mmu_set_page_dir(uint32_t paddr){
    uint32_t cr4=read_cr4();
	write_cr4(cr4|CR4_PSE);
	write_cr3(paddr);
	write_cr0(read_cr0()|CR0_PG);
}

static inline uint32_t pde_index(uint32_t vaddr){
    return vaddr >> 22;
}

static inline uint32_t pte_index(uint32_t vaddr){
    return (vaddr >> 12)  & 0x3FF;
}

static inline uint32_t pde_paddr(pde_t* pde){
    return pde->phy_pt_addr << 12;
}

static inline uint32_t pte_paddr(pte_t* pte){
    return pte->phy_pt_addr << 12;
}

static inline uint32_t get_pte_perm(pte_t* pte){
    return (pte->v & 0x1FF);
}
#endif