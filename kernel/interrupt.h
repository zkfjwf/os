#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "types.h"

// --- 任务3 要求：数据结构定义 ---

// 中断处理函数类型
typedef void (*interrupt_handler_t)(void);

// 中断描述符 (支持共享中断链表)
// 解决 Q3: 如何处理共享中断？ -> 使用链表


// --- 任务3 要求：接口定义 ---
void trap_init(void); 
void register_interrupt(int irq, interrupt_handler_t h, const char *name);
void enable_interrupt(int irq);
void disable_interrupt(int irq);

// 核心分发函数
void interrupt_dispatcher(void);

// 异常与时钟接口
uint64 handle_exception(uint64 scause, uint64 sepc, uint64 stval);
void timer_init(void);

#endif

