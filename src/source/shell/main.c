#include "lib_syscall.h"
#include "main.h"
#include "fs/file.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <sys/file.h>

/// @brief shell的命令行结构体
static cli_t cli;

/// @brief 命令行提示词，在提示词后输入命令
static const char* prompt="sh >>";

/// @brief 存储输入命令缓冲区
char cmd_buf[256];

/**
 * @brief help命令
 * @param argc 参数数量
 * @param argv 参数的字符串 
 */
static int do_help(int argc,char** argv){
    const cli_cmd_t* start=cli.cmd_start;

    while(start<cli.cmd_end){
        printf("%s %s\n",start->name,start->usage);
        start++;
    }

    return 0;
}

/**
 * @brief 实现清屏的函数
 */
static int do_clear(int argc,char** argv){
    printf("%s",ESC_CLEAR_SCREEN);
    printf("%s",ESC_MOVE_CURSOR(0,0));

}

/**
 * @brief echo命令执行的函数
 * @param argc 参数数量
 * @param argv 参数的字符串 
 */
static int do_echo(int argc,char** argv){

    // 当只输入echo命令是等待输入字符串打印
    if(argc == 1){
        char msg_buf[128];
        fgets(msg_buf,sizeof(msg_buf),stdin);
        msg_buf[sizeof(msg_buf)-1]='\0';
        puts(msg_buf);
        return 0;
    }

    int count = 0;
    int ch;
    while((ch=getopt(argc,argv,"n:h")) != -1){
        switch(ch){
            case 'h':
                puts("echo: any message");
                puts("Usage: echo [-n count] msg");
                optind = 1;
                return 0;
            case 'n':
                count = atoi(optarg);
                break;
            case '?':
                if(optarg){
                    fprintf(stderr,"Unknown option: %c\n",ch);
                }
                optind = 1;
                return -1;
            default:
               break;
        }
    }

    if(optind > argc -1){
        fprintf(stderr,"Message is empty\n");
        optind = 1;
        return -1;
    }

    char* msg=argv[optind];
    for(int i=0;i<count;i++){
        puts(msg);
    }
    optind = 1;

    return 0;

}

/**
 * @brief 退出命令行的函数
 * @param argc 参数数量
 * @param argv 参数的字符串 
 */
static int do_exit(int argc,char** argv){
    exit(0);
    return 0;
}

/**
 * @brief ls命令实现的函数
 * @param argc 参数数量
 * @param argv 参数的字符串
 */
static int do_ls(int argc,char** argv){
    DIR* p_dir=opendir("temp");
    if(p_dir == NULL){
        printf("open dir failed\n");
        return -1;
    }

    struct dirent* entry;
    while((entry=readdir(p_dir))!=NULL){
        printf("%c %s %d\n",
            entry->type == FILE_DIR ? 'd' : 'f',
            entry->name,
            entry->size
        );
    }

    closedir(p_dir);
    return 0;
}

/**
 * @brief less命令实现的函数
 * @param argc 参数数量
 * @param argv 参数的字符串
 */
static int do_less(int argc,char **argv){
    int line_mode=0;

    int ch;
    while((ch=getopt(argc,argv,"lh"))!=-1){
        switch(ch){
            case 'h':
                puts("show file content");
                puts("Usage: less [-l] file");
                optind = 1;
                return 0;
            case 'l':
                line_mode = 1;
                break;
            case '?':
                if(optarg){
                    fprintf(stderr,"Unknown option: -%s\n",optarg);
                }
                optind = 1;
                return -1;
            default:
                break;
        }
    }

    if(optind > argc - 1){
        fprintf(stderr,"no file\n");
    }

    FILE* file=fopen(argv[optind],"r");

    if(file == NULL){
        fprintf(stderr,"open file %s failed\n",argv[optind]);
        optind =1;
        return -1;
    }

    char* buf=(char*)malloc(256);
    if(line_mode == 0){
        while(fgets(buf,256,file) != NULL){
            fputs(buf,stdout);
        }
    }
    else{
        // 设置stdin为无缓冲模式
        // 这样可以实时读取用户输入的字符
        setvbuf(stdin,NULL,_IONBF,0);

         // 关闭回显,0x1表示关闭回显TTY_CMD_ECHO这里有些问题就用0x1代替
        ioctl(0,0x1,0,0);
        while(1){
            char *b=fgets(buf,256,file);
            if(b == NULL){
                break;
            }

            fputs(buf,stdout);

            int ch;
            while((ch=fgetc(stdin))!='n'){
                if(ch == 'q'){
                    goto less_quit;
                }
            }
        }
less_quit:
        setvbuf(stdin,NULL,_IOFBF,BUFSIZ); // 恢复stdin的缓冲模式
        ioctl(0,0x1,1,0);
    }

    free(buf);

    fclose(file);
    optind = 1;
    return 0;

}

/**
 * @brief 查找可执行文件的路径
 * @param filename 文件名
 * @return 返回文件的路径，如果没有找到则返回NULL
 */
static const char* find_exec_path(const char* filename){
    int fd=open(filename,0);
    if(fd<0){
        return (const char*)0;
    }

    close(fd);
    return filename;
}

/**
 * @brief 复制文件的函数
 * @param argc 参数数量
 * @param argv 参数的字符串
 */
static int do_cp(int argc,char** argv){
    if(argc < 3){
        fprintf(stderr,"no [from] or [to] file\n");
        return -1;
    }

    FILE *from,*to;

    from=fopen(argv[1],"rb");
    to=fopen(argv[2],"wb");

    if(!from || !to){
        fprintf(stderr,"open file failed\n");
        goto cp_failed;
    }

    char *buf=(char*)malloc(256);
    int size=0;
    while((size=fread(buf,1,256,from))>0){
        fwrite(buf,1,size,to);
    }

    free(buf);

cp_failed:
    if(from){
        fclose(from);
    }

    if(to){
        fclose(to);
    }

    return 0;
}

static int do_rm(int argc,char** argv){
    if(argc < 2){
        fprintf(stderr,"no file\n");
        return -1;
    }

    int err = unlink(argv[1]);
    if(err < 0){
        fprintf(stderr,"remove file %s failed\n",argv[1]);
        return err;
    }

    return 0;
}

/// @brief 命令列表
static const cli_cmd_t cmd_list[]={
    {
        .name="help",
        .usage="help -- list supported command",
        .do_func=do_help
    },
    {
        .name="clear",
        .usage="clear -- clear screen",
        .do_func=do_clear
    },
    {
        .name="echo",
        .usage="echo [-n count] msg -- echo something",
        .do_func=do_echo,
    },
    {
        .name="quit",
        .usage="quit -- quit from shell",
        .do_func=do_exit,
    },
    {
        .name="ls",
        .usage="ls -- list directory",
        .do_func=do_ls,
    },
    {
        .name="less",
        .usage="less [-l] file -- show file content",
        .do_func=do_less,
    },
    {
        .name="cp",
        .usage="cp src dest",
        .do_func=do_cp,
    },
    {
        .name="rm",
        .usage="rm file - remove file",
        .do_func=do_rm,
    }
};


/**
 * @brief 命令行的初始化函数
 * @param prompt 初始化cli_t结构体的prompt成员变量
 * @param cli_cmd_t 命令列表首地址
 * @param size 命令列表包含元素个数
 */
static void cli_init(const char* prompt,const cli_cmd_t* cmd_list,int size){
    cli.prompt=prompt;
    memset(cli.curr_input,0,CLI_INPUT_SIZE);
    cli.cmd_start=cmd_list;
    cli.cmd_end=cmd_list+size;
}

/**
 * @brief 查找内置命令
 * @param name 命令名称
 * @return 返回命令结构体指针
 * @note 如果没有找到命令，返回0
 */
static const cli_cmd_t* find_builtin(const char* name){
    for(const cli_cmd_t* cmd=cli.cmd_start;cmd<cli.cmd_end;cmd++){
        if(strcmp(name,cmd->name)!=0){
            continue;
        }

        return cmd;
    }

    return 0;
}

/**
 * @brief 执行内置命令
 * @param cmd 命令结构体
 * @param argc 参数数量
 * @param argv 参数列表 
 */
static void run_builtin(const cli_cmd_t* cmd,int argc,char** argv){
    int ret=cmd->do_func(argc,argv);
    if(ret<0){
        fprintf(stderr,ESC_COLOR_ERROR"error: %d\n"ESC_COLOR_DEFAULT,ret);
    }
}

/**
 * @brief 执行外部命令
 * @param path 要执行的命令的文件路径
 * @param argc 参数数量
 * @param argv 参数列表 
 */
static void run_exec_file(const char* path,int argc,char** argv){
    int pid=fork();
    if(pid<0){
        fprintf(stderr,ESC_COLOR_ERROR"fork failed %s\n"ESC_COLOR_DEFAULT,path);
        return;
    }
    else if(pid==0){
        int err=execve(path,argv,(char * const *)0);
        if(err<0){
            fprintf(stderr,"exec failed %s",path);
        }
        exit(-1);
    }
    else{
        int status=0;
        int pid=wait(&status);
        fprintf(stderr,"cmd %s result: %d, pid=%d\n",path,pid,status);
    }
}   

/**
 * @brief 显示命令行提示词
 */
static void show_prompt(void){
    printf("%s",cli.prompt);

    // 清空缓存
    fflush(stdout);
}

int main(int argc,char** argv){

    // 打开tty0设备
    open(argv[0],O_RDWR);
    dup(0);
    dup(0);

    cli_init(prompt,cmd_list,sizeof(cmd_list)/sizeof(cmd_list[0]));

    for(;;){

        show_prompt();
        char* str=fgets(cli.curr_input,CLI_INPUT_SIZE,stdin);
        if(!str){
            continue;
        }

        // 去掉换行符
        char* cr=strchr(cli.curr_input,'\n');
        if(cr){
            *cr=0;
        }

        // 去掉回车符
        cr=strchr(cli.curr_input,'\r');
        if(cr){
            *cr=0;
        }


        int argc=0;
        char* argv[CLI_MAX_ARG_COUNT];
        memset(argv,0,sizeof(argv));

        // 分割命令行输入的字符串,解析出命令和参数
        const char* space=" ";
        char* token=strtok(cli.curr_input,space);
        while(token){
            argv[argc++]=token;
            token=strtok(NULL,space);
        }

        if(argc==0){
            continue;
        }

        const cli_cmd_t* cmd=find_builtin(argv[0]);
        if(cmd){
            run_builtin(cmd,argc,argv);
            continue; 
        }

        const char *path=find_exec_path(argv[0]);
        if(path){
            run_exec_file(path,argc,argv);
            continue;
        }

        fprintf(stderr,ESC_COLOR_ERROR"Unknown command: %s\n"ESC_COLOR_DEFAULT,cli.curr_input);
    }

}
