#include "init.h"
#include "core/memory.h"
#include "dev/console.h"
#include "comm/boot_info.h"
#include "dev/time.h"
#include "core/task.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "os_cfg.h"
#include "tools/list.h"
#include "ipc/sem.h"
#include "cpu/cpu.h"
#include "dev/kbd.h"
#include "fs/fs.h"

void kernel_init(boot_info_t* boot_info){
    irq_init();

    cpu_init();
    log_init();

    memory_init(boot_info);
    fs_init();
    
    time_init();

    task_manager_init();

}

void move_to_first_task(void){
    task_t* curr=task_current();
    ASSERT(curr!=0);
    tss_t* tss=&(curr->tss);
    __asm__ __volatile__(
        "push %[ss]\n\t"
        "push %[esp]\n\t"
        "push %[eflags]\n\t"
        "push %[cs]\n\t"
        "push %[eip]\n\t"
        "iret"
        :
        :[ss]"r"(tss->ss),[esp]"r"(tss->esp),[eflags]"r"(tss->eflags),[cs]"r"(tss->cs),
         [eip]"r"(tss->eip)
    );
}

void init_main(){
    int count=0;
    log_printf("Kernel is running....");
    task_first_init();
    move_to_first_task();
}
