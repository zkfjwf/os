// kernel/interrupt.c
#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "interrupt.h"

// =============================================================
// PLIC (S-mode) 物理地址定义 - 针对 QEMU Virt, Hart 0
// =============================================================
#define PLIC_BASE 0x0c000000L
#define PLIC_PRIORITY_BASE     (PLIC_BASE + 0x0)
#define PLIC_SENABLE_BASE      (PLIC_BASE + 0x2080)
#define PLIC_SPRIORITY         (PLIC_BASE + 0x201000)
#define PLIC_SCLAIM            (PLIC_BASE + 0x201004)

#ifndef REG32
#define REG32(addr) (*(volatile uint32 *)(addr))
#endif

//extern void printf(char *fmt, ...);
extern void panic(char *s);
extern void kernelvec(void);

#define MAX_IRQS 64

typedef void (*interrupt_handler_t)(void);

struct irq_action {
    interrupt_handler_t handler;
    struct irq_action *next;
    const char *name;
    int used;
};

static struct irq_action action_pool[MAX_IRQS];
struct irq_action *irq_table[MAX_IRQS];

void trap_init(void) {
    for (int i = 0; i < MAX_IRQS; i++) {
        irq_table[i] = 0;
        action_pool[i].used = 0;
        action_pool[i].next = 0;
        action_pool[i].name = 0;
        action_pool[i].handler = 0;
    }  //清空

    REG32(PLIC_SPRIORITY) = 0;   //允许特权大于0的中断进入

    w_stvec((uint64)kernelvec);  //设置入口

    // 这里只初始化 trap/PLIC，不强行开全局中断（由 start.c 控制也行）
    printf("Trap system initialized.");
}

static struct irq_action* alloc_action_node(void) {
    for (int i = 0; i < MAX_IRQS; i++) {
        if (!action_pool[i].used) {
            action_pool[i].used = 1;  //空闲
            action_pool[i].next = 0;  //串起处理函数
            return &action_pool[i];
        }
    }
    panic("No more IRQ action nodes");
    return 0;
}  //分配一个数组做对象池 存放handler结构

void enable_interrupt(int irq) {
    if (irq <= 0 || irq >= MAX_IRQS) return;

    REG32(PLIC_PRIORITY_BASE + irq * 4) = 1;
    //设优先级为1
    volatile uint32 *reg = (volatile uint32 *)(PLIC_SENABLE_BASE + (irq / 32) * 4);
    *reg |= (1U << (irq % 32));   //置1/清理enable位 图
}

void disable_interrupt(int irq) {
    if (irq <= 0 || irq >= MAX_IRQS) return;

    volatile uint32 *reg = (volatile uint32 *)(PLIC_SENABLE_BASE + (irq / 32) * 4);
    *reg &= ~(1U << (irq % 32));
}

void register_interrupt(int irq, interrupt_handler_t h, const char *name) {
    if (irq <= 0 || irq >= MAX_IRQS) panic("Invalid IRQ");

    struct irq_action *node = alloc_action_node();
    node->handler = h;
    node->name = name;
    node->next = 0;

    if (irq_table[irq] == 0) {
        irq_table[irq] = node;
    } else {
        struct irq_action *p = irq_table[irq];
        while (p->next) p = p->next;
        p->next = node;
    }

    enable_interrupt(irq);
}

void interrupt_dispatcher(void) {
    int irq = REG32(PLIC_SCLAIM);

    if (irq) {
        struct irq_action *p = irq_table[irq];

        if (p) {
            while (p) {
                if (p->handler) p->handler();
                p = p->next;
            }
        } else {
            printf("Unexpected IRQ: %d", irq);
        }

        REG32(PLIC_SCLAIM) = irq;
    }
}


