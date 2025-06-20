#include "core/task.h"
#include "core/memory.h"
#include "cpu/mmu.h"
#include "core/syscall.h"
#include "comm/elf.h"
#include "fs/fs.h"

/// @brief 任务管理器
static task_manager_t task_manager;

/// @brief idle_task的栈
static uint32_t idle_task_stack[IDLE_TASK_SIZE];

/// @brief 任务表用来存储所有的任务
static task_t task_table[TASK_NR];

/// @brief 互斥锁，用来保护任务表的访问
static mutex_t table_mutex;

/**
 * @brief 初始化任务的TSS结构体
 * @param task TSS结构体对应的任务结构体
 * @param flag 任务的标志位，设置任务的特权级
 * @param entry 任务的入口地址
 * @param esp 任务的栈顶地址
 * @return 0 成功，-1 失败
 */
static int tss_init(task_t* task,int flag,uint32_t entry,uint32_t esp){
    int tss_sel=gdt_alloc_desc();
    if(tss_sel<0){
        log_printf("alloc tss failed.\n");
        return -1;
    }

    // 这里将tss_sel设置为特权级0的TSS段描述符为了防止任意修改tss
    segment_desc_set(tss_sel,(uint32_t)&task->tss,sizeof(tss_t),
        SEG_P_PRESENT | SEG_DPL0 | SEG_TYPE_TSS
    );
    kernel_memset(&task->tss,0,sizeof(tss_t));

    uint32_t kernel_stack=memory_alloc_page();
    if(kernel_stack == 0){
        goto tss_init_failed;
    }
    
    int code_sel,data_sel;
    if(flag & TASK_FLAGS_SYSTEM){
        code_sel=KERNEL_SELECTOR_CS;
        data_sel=KERNEL_SELECTOR_DS;
    }
    else{
        code_sel=task_manager.app_code_sel | SEG_CPL3;
        data_sel=task_manager.app_data_sel | SEG_CPL3;
    }

    task->tss.eip=entry;
    task->tss.esp=esp;
    task->tss.esp0=kernel_stack+MEM_PAGE_SIZE;
    task->tss.ss=data_sel;
    task->tss.ss0=KERNEL_SELECTOR_DS;
    task->tss.es=task->tss.ds=task->tss.fs=task->tss.gs=data_sel;
    task->tss.cs=code_sel;
    task->tss.eflags=EFLAGS_DEFAULT | EFLAGS_IF;

    uint32_t page_dir=memory_create_uvm();
    if(page_dir == 0){
        goto tss_init_failed;
    }
    task->tss.cr3=page_dir;
    task->tss_sel=tss_sel;
    return 0;
tss_init_failed:
    gdt_free_sel(tss_sel);
    if(kernel_stack){
        memory_free_page(kernel_stack);
    }
    return -1;

}

int task_init(task_t* task,const char*name,int flag,uint32_t entry,uint32_t esp){
    ASSERT(task!=(task_t*)0);
    int err=tss_init(task,flag,entry,esp);
    if(err<0){
        log_printf("init task failed.");
        return err;
    }
    kernel_strncpy(task->name,name,TASK_NAME_SIZE);

    task->state=TASK_CREATED;
    task->time_ticks=TASK_TIME_SLICE_DEFAULT;
    task->slice_ticks=task->time_ticks;
    task->sleep_ticks=0;
    task->status=0;
    
    list_node_init(&task->all_node);
    list_node_init(&task->run_node);
    list_node_init(&task->wait_node);
    
    irq_state_t state=irq_enter_protection();

    task->pid=(uint32_t)task;
    task->parent=(task_t*)0;
    task->heap_start=0;
    task->heap_end=0;

    // 对task->file_table进行初始化
    kernel_memset(&task->file_table,0,sizeof(task->file_table));

    list_insert_last(&task_manager.task_list,&task->all_node);

    irq_leave_protection(state);

    return 0;
}

void task_start(task_t* task){
    irq_state_t state=irq_enter_protection();
    task_set_ready(task);
    irq_leave_protection(state);
}

/**
 * @brief 释放任务的资源
 * @param task 需要释放的任务结构体
 */
void task_uninit(task_t* task){
    if(task->tss_sel){
        gdt_free_sel(task->tss_sel);
    }

    if(task->tss.esp0){
        memory_free_page(task->tss.esp-MEM_PAGE_SIZE);
    }

    if(task->tss.cr3){
        memory_destroy_uvm(task->tss.cr3);
    }

    kernel_memset(task,0,sizeof(task));
}

void task_switch_from_to(task_t*from,task_t*to){
    switch_to_tss(to->tss_sel);
}

static void idle_task_entry(void){
    for(;;){
        hlt();
    }
}

/**
 * @brief 初始化任务管理器
 */
void task_manager_init(void){

    kernel_memset(task_table,0,sizeof(task_table));
    mutex_init(&table_mutex);

    int sel=gdt_alloc_desc();

    // 这里虽然设置的代码段和数据段范围还是0x0-0xFFFFFFFF，但使用该段的段选择子访问的方式为特权级3
    segment_desc_set(sel,0x0,0xFFFFFFFF,
        SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL | SEG_TYPE_DATA | SEG_TYPE_RW | SEG_D
    );
    task_manager.app_data_sel=sel;

    sel=gdt_alloc_desc();
    segment_desc_set(sel,0x0,0xFFFFFFFF,
           SEG_P_PRESENT | SEG_DPL3 | SEG_S_NORMAL |SEG_TYPE_CODE | SEG_TYPE_RW | SEG_D
    );
    task_manager.app_code_sel=sel;

    list_init(&task_manager.ready_list);
    list_init(&task_manager.task_list);
    list_init(&task_manager.sleep_list);
    task_manager.curr_task=(task_t*)0;
    task_init(&task_manager.idle_task,"idle_task",TASK_FLAGS_SYSTEM,(uint32_t)idle_task_entry,
    (uint32_t)(idle_task_stack+IDLE_TASK_SIZE));

    task_start(&task_manager.idle_task);
}

/**
 * @brief 初始化第一个任务
 * @note 该函数的运行的入口地址为first_task_entry，位于first_task_entry.S中
 */
void task_first_init(void){
    void first_task_entry(void);
    extern uint8_t s_first_task[],e_first_task[];

    uint32_t copy_size=(uint32_t)(e_first_task-s_first_task);
    uint32_t alloc_size=10*MEM_PAGE_SIZE;
    ASSERT(copy_size < alloc_size);

    uint32_t first_start=(uint32_t)first_task_entry;

    task_init(&task_manager.first_task,"first task",0,first_start,first_start+alloc_size);

    task_manager.first_task.heap_start=(uint32_t)e_first_task;
    task_manager.first_task.heap_end=(uint32_t)e_first_task;

    write_tr(task_manager.first_task.tss_sel);
    task_manager.curr_task=&task_manager.first_task;

    mmu_set_page_dir(task_manager.first_task.tss.cr3);

    memory_alloc_page_for(first_start,alloc_size,PTE_P | PTE_W | PTE_U);
    kernel_memcpy((void*)first_start,s_first_task,copy_size);

    task_start(&task_manager.first_task);
}

task_t* task_first_task(void){
    return &task_manager.first_task;
}

/**
 * @brief 设置任务为就绪状态
 * @param task 需要设置的任务
 * @return 0 成功，-1 失败
 * @note 该函数会将任务插入到就绪队列中，将任务的状态设置为就绪
 */
void task_set_ready(task_t* task){
    if(task==&task_manager.idle_task){
        return;
    }
    list_insert_last(&task_manager.ready_list,&task->run_node);
    task->state=TASK_READY;
}

void task_set_block(task_t* task){
    if(task==&task_manager.idle_task){
        return;
    }
    list_remove(&task_manager.ready_list,&task->run_node);
}

task_t* task_next_run(void){
    if(list_count(&task_manager.ready_list)==0){
        return &task_manager.idle_task;
    }
    list_node_t* task_node=list_first(&task_manager.ready_list);
    return list_node_parent(task_node,task_t,run_node);
}

task_t* task_current(void){
    return task_manager.curr_task;
}

int sys_sched_yield(void){
    irq_state_t state=irq_enter_protection();
    if(list_count(&task_manager.ready_list)>1){
        task_t* curr_task=task_current();

        task_set_block(curr_task);
        task_set_ready(curr_task);

        task_dispatch();
    }
    irq_leave_protection(state);
    return 0;
}

void task_dispatch(void){
    irq_state_t state=irq_enter_protection();
    task_t* to=task_next_run();
    if(to!=task_manager.curr_task){
        task_t* from=task_current();
        task_manager.curr_task=to;
        to->state=TASK_RUNNING;
        task_switch_from_to(from,to);
    }
    irq_leave_protection(state);
}

void task_time_tick(void){
    irq_state_t state=irq_enter_protection();
    task_t* curr_task=task_current();
    if(--curr_task->slice_ticks==0){
        curr_task->slice_ticks=curr_task->time_ticks;
        task_set_block(curr_task);
        task_set_ready(curr_task);
    }
    list_node_t* curr=list_first(&task_manager.sleep_list);
    while(curr){
        list_node_t*next=list_node_next(curr);
        task_t* task=list_node_parent(curr,task_t,run_node);
        if(--task->sleep_ticks==0){
            task_set_wakeup(task);
            task_set_ready(task);
        }
        curr=next;
    }
    task_dispatch();
    irq_leave_protection(state);
}

void task_set_sleep(task_t* task,uint32_t ticks){
    if(ticks==0){
        return;
    }
    task->sleep_ticks=ticks;
    task->state=TASK_SLEEP;
    list_insert_last(&task_manager.sleep_list,&task->run_node);
}

void task_set_wakeup(task_t* task){
    list_remove(&task_manager.sleep_list,&task->run_node);
}

void sys_msleep (uint32_t ms) {
    if (ms < OS_TICK_MS) {
        ms = OS_TICK_MS;
    }

    irq_state_t state = irq_enter_protection();

    task_set_block(task_manager.curr_task);
    task_set_sleep(task_manager.curr_task, (ms + (OS_TICK_MS - 1))/ OS_TICK_MS);
    
    task_dispatch();

    irq_leave_protection(state);
}

int sys_getpid(void){
    task_t* task=task_current();
    return task->pid;
}

static task_t* alloc_task(void){
    task_t* task=(task_t*)0;
    mutex_lock(&table_mutex);
    for(int i=0;i<TASK_NR;i++){
        task_t* curr=task_table+i;
        if(curr->pid==0){
            task=curr;
            break;
        }
    }   
    mutex_unlock(&table_mutex);

    return task;
}

static void free_task(task_t* task){
    mutex_lock(&table_mutex);
    task->pid=0;
    mutex_unlock(&table_mutex);
}

/**
 * @brief 复制父进程打开的文件到子进程
 * @param child_task 子进程
 */
static void copy_opened_files(task_t* child_task){
    task_t* parent=task_current();
    for(int i=0;i<TASK_OFILE_NR;i++){

        file_t* file=parent->file_table[i];
        if(file){
            file_inc_ref(file);
            child_task->file_table[i]=file;
        }
    }
}

/**
 * @brief 创建一个新的任务
 */
int sys_fork(void){
    task_t* parent_task=task_current();
    task_t* child_task=alloc_task();

    if(child_task==(task_t*)0){
        goto fork_failed;
    }

    syscall_frame_t* frame=(syscall_frame_t*)(parent_task->tss.esp0-sizeof(syscall_frame_t));
    int err=task_init(child_task,parent_task->name,0,frame->eip,frame->esp+sizeof(uint32_t)*SYSCALL_PARAM_COUNT);
    if(err < 0){
        goto fork_failed;
    }

    copy_opened_files(child_task);

    tss_t* tss=&child_task->tss;
    tss->eax= 0;
    tss->ebx=frame->ebx;
    tss->ecx=frame->ecx;
    tss->edx=frame->edx;
    tss->esi=frame->esi;
    tss->edi=frame->edi;
    tss->ebp=frame->ebp;

    tss->cs=frame->cs;
    tss->ds=frame->ds;
    tss->es=frame->es;
    tss->fs=frame->fs;
    tss->gs=frame->gs;
    tss->eflags=frame->eflags;

    child_task->parent=parent_task;

    if((tss->cr3=memory_copy_uvm(parent_task->tss.cr3))<0){
        goto fork_failed;
    }

    task_start(child_task);

    return child_task->pid;

fork_failed:
    if(child_task){
        task_uninit(child_task);
        free_task(child_task);
    }

    return -1;
}

static int load_phdr(int file,Elf32_Phdr*phdr ,uint32_t page_dir){
    int err=memory_alloc_for_page_dir(page_dir,phdr->p_vaddr,phdr->p_memsz,PTE_P|PTE_U|PTE_W);
    if(err < 0){
        log_printf("no memory");
        return -1;
    }

    if(sys_lseek(file,phdr->p_offset,0)<0){
        log_printf("read file failed");
        return -1;
    }

    uint32_t vaddr=phdr->p_vaddr;
    uint32_t size=phdr->p_filesz;

    while(size > 0){
        int curr_size=(size > MEM_PAGE_SIZE) ? MEM_PAGE_SIZE : size;
        uint32_t paddr=memory_get_paddr(page_dir,vaddr);

        if(sys_read(file,(char*)paddr,curr_size)<curr_size){
            log_printf("read file failed.");
            return -1;
        }

        size-=curr_size;
        vaddr+=curr_size;
    }

    return 0;

}

static uint32_t load_elf_file(task_t* task,const char* name,uint32_t page_dir){
    Elf32_Ehdr elf_hdr;
    Elf32_Phdr elf_phdr;

    int file=sys_open(name,0);
    if(file<0){
        log_printf("open failed. %s",name);
        goto load_failed;
    }

    int cnt=sys_read(file,(char*)&elf_hdr,sizeof(elf_hdr));
    if(cnt<sizeof(Elf32_Ehdr)){
        log_printf("elf hdr too small. size=%d",cnt);
        goto load_failed;
    }

    if((elf_hdr.e_ident[0]!=0x7f)||(elf_hdr.e_ident[1]!='E')
		|| (elf_hdr.e_ident[2]!='L')||(elf_hdr.e_ident[3]!='F')){
            log_printf("check elf ident failed.");
			goto load_failed;
	}

    uint32_t e_phoff=elf_hdr.e_phoff;
    for(int i=0;i<elf_hdr.e_phnum;i++,e_phoff+=elf_hdr.e_phentsize){
        if(sys_lseek(file,e_phoff,0)<0){
            log_printf("read file failed.");
            goto load_failed;
        }

        cnt=sys_read(file,(char*)&elf_phdr,sizeof(elf_phdr));
        if(cnt<sizeof(elf_phdr)){
            log_printf("read file failed.");
            goto load_failed;
        }

        if((elf_phdr.p_type!=1)|| (elf_phdr.p_vaddr<MEMORY_TASK_BASE)){
            continue;
        }

        int err=load_phdr(file,&elf_phdr,page_dir);
        if(err<0){
            log_printf("load program failed.");
            goto load_failed;
        }

        task->heap_start=elf_phdr.p_vaddr+elf_phdr.p_memsz;
        task->heap_end=task->heap_start;
    }


    sys_close(file);
    return elf_hdr.e_entry;

load_failed:
    if(file){
        sys_close(file);
    }
    return 0;
}

static int copy_args(char* to,uint32_t page_dir,int argc,char**argv){
    task_args_t task_args;
    task_args.argc=argc;
    task_args.argv=(char**)(to+sizeof(task_args_t));

    char* dest_arg=to+sizeof(task_args_t)+sizeof(char*)*argc;
    char** dest_argv_tb=(char**)memory_get_paddr(page_dir,(uint32_t)(to+sizeof(task_args_t)));

    for(int i=0;i<argc;i++){
        char* from=argv[i];
        int len=kernel_strlen(from)+1;
        int err=memory_copy_uvm_data((uint32_t)dest_arg,page_dir,(uint32_t)from,len);
        ASSERT(err >= 0);

        dest_argv_tb[i]=dest_arg;
        dest_arg+=len;

    }

    // if(argc > 0){
    //     //dest_argv_tb[argc]='\0';
    // }
    
    return memory_copy_uvm_data((uint32_t)to,page_dir,(uint32_t)&task_args,sizeof(task_args));

}

int sys_execve(char* name,char** argv,char** env){
    task_t* task=task_current();

    kernel_strncpy(task->name,get_file_name(name),TASK_NAME_SIZE);

    uint32_t old_page_dir=task->tss.cr3;
    uint32_t new_page_dir=memory_create_uvm();

    if(!new_page_dir){
        goto exec_failed;
    }

    uint32_t entry=load_elf_file(task,name,new_page_dir);

    if(entry==0){
        goto exec_failed;
    }

    uint32_t stack_top=MEM_TASK_STACK_TOP-MEM_TASK_ARG_SIZE;

    int err=memory_alloc_for_page_dir(new_page_dir,
        MEM_TASK_STACK_TOP-MEM_TASK_STACK_SIZE,MEM_TASK_STACK_SIZE,
        PTE_P | PTE_U | PTE_W
    );

    if(err < 0){
        goto exec_failed;
    }
    
    int argc=string_count(argv);
    err=copy_args((char*)stack_top,new_page_dir,argc,argv);
    if(err<0){
        goto exec_failed;
    }

    syscall_frame_t* frame=(syscall_frame_t*)(task->tss.esp0-sizeof(syscall_frame_t));
    frame->eip=entry;
    frame->eax=frame->ebx=frame->edx=0;
    frame->esi=frame->edi=frame->ebp=0;
    frame->eflags=EFLAGS_DEFAULT|EFLAGS_IF;

    frame->esp=stack_top-sizeof(uint32_t)*SYSCALL_PARAM_COUNT;

    task->tss.cr3=new_page_dir;
    mmu_set_page_dir(new_page_dir);

    memory_destroy_uvm(old_page_dir);

    return 0;

exec_failed:
    if(new_page_dir){
        task->tss.cr3=old_page_dir;
        mmu_set_page_dir(old_page_dir);

        memory_destroy_uvm(new_page_dir);
    }
    return -1;
}

/**
 * @brief 获取文件描述符对应的文件
 * @param fd 文件描述符
 * @return 获取成功返回对应的文件指针，不成功返回NULL
 */
file_t* task_file(int fd){
    if((fd>=0) && (fd<TASK_OFILE_NR)){
        file_t* file=task_current()->file_table[fd];
        return file;
    }

    return (file_t*)0;
}

/**
 * @brief 给文件分配描述符
 * @param file 指向一个文件的指针
 * @return 如果分配成功返回对应的fd如果分配失败返回-1
 */
int task_alloc_fd(file_t* file){
    task_t* task=task_current();

    for(int i=0;i<TASK_OFILE_NR;i++){
        file_t* p=task->file_table[i];
        if(p==(file_t*)0){
            task->file_table[i]=file;
            return i;
        }
    }

    return -1;
}

/**
 * @brief 释放一个文件描述符
 * @param fd 要释放的文件描述符
 */
void task_remove_fd(int fd){
   if((fd>=0) && (fd<TASK_OFILE_NR)){
       task_current()->file_table[fd]=(file_t*)0;
    }
}

void sys_exit(int status){
    task_t* curr_task=task_current();

    for(int fd=0;fd<TASK_OFILE_NR;fd++){
        file_t* file=curr_task->file_table[fd];
        if(file){
            sys_close(fd);
            curr_task->file_table[fd]=(file_t*)0;
        }
    }

    int move_child=0;

    mutex_lock(&table_mutex);

    for(int i=0;i<TASK_NR;i++){
        task_t* task=task_table+i;
        if(task->parent==curr_task){
            task->parent=&task_manager.first_task;
            if(task->state==TASK_ZOMBIE){
                move_child=1;  
            }
        }
    }

    mutex_unlock(&table_mutex);

    irq_state_t state=irq_enter_protection();

    task_t* parent=curr_task->parent;
    if(move_child && (parent!=&task_manager.first_task)){
        if(task_manager.first_task.state==TASK_WAITING){
            task_set_ready(&task_manager.first_task);
        }
    }

    if(parent->state==TASK_WAITING){
        task_set_ready(curr_task->parent);
    }

    curr_task->status=status;
    curr_task->state=TASK_ZOMBIE;
    task_set_block(curr_task);

    irq_leave_protection(state);
    
    task_dispatch();


}

/**
 * @brief 等待子进程结束
 * @param status 退出状态码
 * @return 返回子进程的pid
 */
int sys_wait(int* status){
    task_t* curr_task=task_current();

    // 注意这个for(;;){}的位置
    for(;;){
        mutex_lock(&table_mutex);

        for(int i=0;i<TASK_NR;i++){
            task_t* task=task_table+i;
            if(task->parent != curr_task){
                continue;
            }    

            if(task->state==TASK_ZOMBIE){
                int pid=task->pid;
                *status=task->status;
                
                task_uninit(task);
                kernel_memset(task,0,sizeof(task_t));

                mutex_unlock(&table_mutex);

                return pid;
            }
        }

        irq_state_t state=irq_enter_protection();

        task_set_block(curr_task);
        curr_task->state=TASK_WAITING;

        irq_leave_protection(state);

        mutex_unlock(&table_mutex);

        task_dispatch();
    }
}