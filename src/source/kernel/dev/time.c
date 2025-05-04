#include "dev/time.h"

// 定时器计数
static uint32_t sys_tick;

void do_handler_time(exception_frame_t* frame){
    sys_tick++;
    pic_send_eoi(IRQ0_TIMER);
    task_time_tick();
}

// 定时器硬件初始化
static void init_pit(void){
    
    uint32_t reload_count = PIT_OSC_FREQ / (1000.0 / OS_TICK_MS);
    // 设置多少秒产生中断
    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNEL0 | PIT_LOAD_LOHI | PIT_MODE3);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF);   // 加载低8位
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xFF); // 再加载高8位

    irq_install(IRQ0_TIMER, (irq_handler_t)exception_handler_time);
    irq_enable(IRQ0_TIMER);
}

// 定时器初始化
void time_init(void){
    sys_tick=0;
    init_pit();
}