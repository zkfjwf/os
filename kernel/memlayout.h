#ifndef MEMLAYOUT_H
#define MEMLAYOUT_H

// QEMU virt机器的物理内存起始地址
#define KERNBASE 0x80000000L
#define PHYSTOP  (KERNBASE + 128*1024*1024) // 128MB RAM

// --- VirtIO 磁盘 ---
#define VIRTIO0 0x10001000

// --- UART 串口 ---
#define UART0 0x10000000

// --- CLINT (Core Local Interruptor) ---
// 用于生成时钟中断 (Timer Interrupt)
#define CLINT 0x2000000L
#define CLINT_MTIMECMP(hartid) (CLINT + 0x4000 + 8*(hartid))
#define CLINT_MTIME (CLINT + 0xBFF8) // 机器时间寄存器

// --- PLIC (Platform Level Interrupt Controller) ---
// 用于管理外部中断 (如UART, 磁盘)
#define PLIC_BASE 0x0c000000L

// PLIC 寄存器地址计算宏
// 1. 优先级寄存器 (Priority): 每个中断源一个，设置优先级(0-7)
#define PLIC_PRIORITY(irq) (PLIC_BASE + (irq) * 4)

// 2. 挂起寄存器 (Pending): 只读，表示中断是否发生
#define PLIC_PENDING(irq) (PLIC_BASE + 0x1000 + ((irq) / 32) * 4)

// 3. 使能寄存器 (Enable): 针对每个Hart的每个模式(M/S)
// QEMU virt中，Hart 0 是 M-mode，Context 1 开始通常用于 S-mode
// 这里假设单核运行，使用 Context 1 (Hart 0 S-mode)
#define PLIC_SENABLE(irq) (PLIC_BASE + 0x2080 + ((irq) / 32) * 4)

// 4. 阈值与声明寄存器 (Threshold & Claim/Complete)
// Context 1 (Hart 0 S-mode)
#define PLIC_SPRIORITY_THRESHOLD (PLIC_BASE + 0x201000)
#define PLIC_SCLAIM (PLIC_BASE + 0x201004)

#endif
