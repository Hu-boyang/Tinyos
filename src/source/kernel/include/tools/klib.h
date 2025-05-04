#ifndef KLIB_H
#define KLIB_H
#include <stdarg.h>
#include "comm/types.h"
#include "tools/log.h"
#include "comm/cpu_instr.h"

static inline uint32_t down2(uint32_t size,uint32_t bound){
    return size & ~(bound-1);
}

static inline uint32_t up2(uint32_t size,uint32_t bound){
    return (size+bound-1) & ~(bound-1);
}

// 复制字符串
void kernel_strcpy(char* dest,const char* src);
// 复制字符串指明复制多个字符
void kernel_strncpy(char* dest,const char* src,unsigned int size);
// 比较两个字符串
int kernel_strncmp(const char* s1,const char* s2,unsigned int size);
// 获取字符串长度
int kernel_strlen(const char* str);
// 复制内存的值
void kernel_memcpy(void* dest,void* src,unsigned int size);
// 设置内存的值
void kernel_memset(void* dest,uint8_t v,int size);
// 比较内存的值
int kernel_memcmp(void* d1,void* d2,unsigned int size);

// 格式化字符串
void kernel_vsprintf(char* buf,const char* fmt,va_list args);

void kernel_sprintf(char* buf,const char* fmt,...);

// int to ASCII 这里base代表处理的数为多少进制
void kernel_itoa(char* buf,int num,int base);


// 用于打印错误信息
#ifndef RELEASE
#define ASSERT(condition)    \
    if (!(condition)) panic(__FILE__, __LINE__, __func__, #condition)
void panic (const char * file, int line, const char * func, const char * cond);
#else
#define ASSERT(condition)    ((void)0)
#endif

int string_count(char**start);
char* get_file_name(char*name);
#endif