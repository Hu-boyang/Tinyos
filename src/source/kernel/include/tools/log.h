#ifndef LOG_H
#define LOG_H

#include "comm/cpu_instr.h"
#include "tools/klib.h"
#include "cpu/irq.h"


#define COM1_PORT 0x3F8

void log_init(void);
void log_printf(const char*fmt,...);
#endif