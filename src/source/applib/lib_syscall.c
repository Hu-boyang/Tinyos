#include "lib_syscall.h"
#include "comm/types.h"
#include "core/syscall.h"
#include "os_cfg.h"

#include <stdlib.h>
#include <string.h>

static inline int sys_call(syscall_args_t*args){
    uint32_t addr[]={0,SELECTOR_SYSCALL | 0};
    int ret;
    __asm__ __volatile__(
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"
        "lcall *(%[a])"
        :"=a"(ret)
        :[a]"r"(addr),
         [arg3]"r"(args->arg3),
         [arg2]"r"(args->arg2),
         [arg1]"r"(args->arg1),
         [arg0]"r"(args->arg0),
         [id]"r"(args->id)
    );
    return ret;
}

void msleep(int ms){
    if(ms<=0){
        return;
    }

    syscall_args_t args;
    args.id=SYS_SLEEP;
    args.arg0=ms;

    sys_call(&args);
}

int getpid(void){
    syscall_args_t args;
    args.id=SYS_GETPID;
    return sys_call(&args);
}

void print_msg(const char* fmt,int arg){
    syscall_args_t args;
    args.id=SYS_PRINT_MSG;
    args.arg0=(uint32_t)fmt;
    args.arg1=arg;
    sys_call(&args);
}

int fork(void){
    syscall_args_t args;
    args.id=SYS_FORK;
    return sys_call(&args);
}

int execve(const char* name,char* const* argv,char* const* env){
    syscall_args_t args;
    args.id=SYS_EXECVE;
    args.arg0=(int)name;
    args.arg1=(int)argv;
    args.arg2=(int)env;

    sys_call(&args);
}

int yield(void){
    syscall_args_t args;
    args.id=SYS_YIELD;

    return sys_call(&args);
}

int open(const char*name,int flags, ...){
    syscall_args_t args;
    args.id=SYS_OPEN;
    args.arg0=(int)name;
    args.arg1=(int)flags;

    return sys_call(&args);
}

int read(int file,char* ptr,int len){
    syscall_args_t args;
    args.id=SYS_READ;
    args.arg0=(int)file;
    args.arg1=(int)ptr;
    args.arg2=(int)len;

    return sys_call(&args);
}

int write(int file,char*ptr,int len){
    syscall_args_t args;
    args.id=SYS_WRITE;
    args.arg0=(int)file;
    args.arg1=(int)ptr;
    args.arg2=(int)len;

    return sys_call(&args);
}

int close(int file){
    syscall_args_t args;
    args.id=SYS_CLOSE;
    args.arg0=(int)file;

    return sys_call(&args);
}

int lseek(int file,int ptr,int dir){
    syscall_args_t args;
    args.id=SYS_LSEEK;
    args.arg0=(int)file;
    args.arg1=(int)ptr;
    args.arg2=(int)dir;
    return sys_call(&args);
}

int isatty(int file){
    syscall_args_t args;
    args.id=SYS_ISATTY;
    args.arg0=(int)file;

    return sys_call(&args);
}

int fstat(int file,struct stat* st){
    syscall_args_t args;
    args.id=SYS_FSTAT;
    args.arg0=(int)file;
    args.arg1=(int)st;

    return sys_call(&args);
}

void* sbrk(ptrdiff_t incr){
    syscall_args_t args;
    args.id=SYS_SBRK;
    args.arg0=(int)incr;

    return (void*)sys_call(&args);
}

/** 
* @brief 复制文件描述符
* @param file 需要复制的文件描述符
* @return 复制后的文件描述符，失败返回-1
*/
int dup(int file){
    syscall_args_t args;
    args.id=SYS_DUP;
    args.arg0=(int)file;

    return sys_call(&args);
}

/**
 * @brief  当我们使用newlib库时，exit函数会调用_exit函数
 * @param status 退出的状态码
 */
void _exit(int status){
    syscall_args_t args;
    args.id=SYS_EXIT;
    args.arg0=(int)status;

    sys_call(&args);

    for(;;){
        asm volatile("hlt");
    }
}

int wait(int* status){
    syscall_args_t args;
    args.id=SYS_WAIT;
    args.arg0=(int)status;

    return sys_call(&args);
}

DIR *opendir(const char* path){
    DIR *dir = (DIR*)malloc(sizeof(DIR));
    if(dir == NULL){
        return NULL;
    }

    syscall_args_t args;
    args.id=SYS_OPENDIR;
    args.arg0=(int)path;
    args.arg1=(int)dir;

    int err=sys_call(&args);
    if(err < 0){
        free(dir);
        return NULL;
    }

    return dir;
}

struct dirent *readdir(DIR *dir){
   syscall_args_t args;

    args.id=SYS_READDIR;
    args.arg0=(int)dir;
    args.arg1=(int)&dir->dirent;

    int err=sys_call(&args);
    if(err < 0){
        return NULL;
    }

    return &dir->dirent;
}

int closedir(DIR *dir){
    syscall_args_t args;

    args.id=SYS_CLOSEDIR;
    args.arg0=(int)dir;

    int err=sys_call(&args);
    if(err < 0){
        return -1;
    }

    free(dir);
    return 0;
}

int ioctl(int file,int cmd,int arg0,int arg1){
    syscall_args_t args;

    args.id=SYS_IOCTL;

    args.arg0=(int)file;
    args.arg1=(int)cmd;
    args.arg2=(int)arg0;
    args.arg3=(int)arg1;

    return sys_call(&args);
}

int unlink(const char *path){
    syscall_args_t args;

    args.id=SYS_UNLINK;
    args.arg0=(int)path;

    return sys_call(&args);
}