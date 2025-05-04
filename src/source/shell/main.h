#ifndef MAIN_H
#define MAIN_H

/// @brief  command line interface 命令行输入的大小
#define CLI_INPUT_SIZE  1024

/// @brief  一条命令的最大参数个数
#define CLI_MAX_ARG_COUNT  10

/**
 * @brief 根据Pn和cmd生成指定的ANSI终端转义序列命令
 * @param Pn  参数
 * @param cmd 命令
 */
#define ESC_CMD2(Pn, cmd) "\x1b["#Pn#cmd

/// @brief  将错误命令前景色设置为红色
#define ESC_COLOR_ERROR ESC_CMD2(31,m)

/// @brief 将背景色重新设置为黑色
#define ESC_COLOR_DEFAULT ESC_CMD2(39,m)

/// @brief  调用ESC_CMD2实现清屏
#define ESC_CLEAR_SCREEN ESC_CMD2(2,J)

/**
 * @brief 移动光标位置的宏定义
 * @param row 要移动到的行号
 * @param col 要移动到的列号
 */
#define ESC_MOVE_CURSOR(row,col) "\x1b["#row";"#col"H"

/**
 * @brief 描述单个命令的结构体
 * @param name 命令名称
 * @param usage 命令的使用方法
 * @param do_func 命令的处理函数
 */
typedef struct _cli_cmd_t{
    const char* name;
    const char* usage;
    int (*do_func)(int argc,char** argv);
}cli_cmd_t;


/**
 * @brief 描述命令行的结构体
 * @param curr_input 输入命令缓冲区
 * @param cmd_start 命令列表的起始地址
 * @param cmd_end 命令列表的结束地址
 * @param prompt 指向特定字符串，每次按下回车键显示该字符串
 */
typedef struct _cli_t{
    char curr_input[CLI_INPUT_SIZE];
    const cli_cmd_t* cmd_start;
    const cli_cmd_t* cmd_end;
    const char* prompt;
}cli_t;


#endif