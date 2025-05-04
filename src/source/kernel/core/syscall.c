#include "core/syscall.h"
#include "core/task.h"
#include "fs/fs.h"
#include "core/memory.h"

/// @brief 系统调用的函数指针，统一以这种方式定义
typedef int (*syscall_handler_t)(uint32_t arg0,uint32_t arg1,uint32_t arg2,uint32_t arg3);


void sys_print_msg(char* fmt,int arg){
    log_printf(fmt,arg);
}

/// @brief 系统调用函数表，函数号和函数指针的映射关系
static const syscall_handler_t sys_table[]={
    [SYS_SLEEP]=(syscall_handler_t)sys_msleep,
    [SYS_GETPID]=(syscall_handler_t)sys_getpid,
    [SYS_FORK]=(syscall_handler_t)sys_fork,
    [SYS_EXECVE]=(syscall_handler_t)sys_execve,
    [SYS_YIELD]=(syscall_handler_t)sys_sched_yield,
    [SYS_OPEN]=(syscall_handler_t)sys_open,
    [SYS_READ]=(syscall_handler_t)sys_read,
    [SYS_WRITE]=(syscall_handler_t)sys_write,
    [SYS_CLOSE]=(syscall_handler_t)sys_close,
    [SYS_LSEEK]=(syscall_handler_t)sys_lseek,
    [SYS_ISATTY]=(syscall_handler_t)sys_isatty,
    [SYS_SBRK]=(syscall_handler_t)sys_sbrk,
    [SYS_FSTAT]=(syscall_handler_t)sys_fstat,
    [SYS_DUP]=(syscall_handler_t)sys_dup,
    [SYS_EXIT]=(syscall_handler_t)sys_exit,
    [SYS_WAIT]=(syscall_handler_t)sys_wait,
    
    [SYS_PRINT_MSG]=(syscall_handler_t)sys_print_msg,
};


void do_handler_syscall(syscall_frame_t*frame){
    if(frame->func_id < sizeof(sys_table)/sizeof(sys_table[0])){
        syscall_handler_t handler=sys_table[frame->func_id];
        if(handler){
            int ret=handler(frame->arg0,frame->arg1,frame->arg2,frame->arg3);
            frame->eax=ret;
            return;
        }
    }

    task_t* task=task_current();
    log_printf("task: %s, Unknown syscall: %d",task->name,frame->func_id);
    frame->eax=-1;
}