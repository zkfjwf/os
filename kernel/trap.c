#include "types.h"
#include "proc.h"
#include "riscv.h"
#include "defs.h"
#include "interrupt.h"

#define TIMER_INTERVAL_CYCLES 1000000ULL

volatile int *timer_interrupt_hook = 0;
static int illegal_seen = 0;
static int pf_seen = 0;
volatile int g_illegal_seen = 0;
volatile int g_loadpf_seen = 0;
volatile uint64 g_expected_loadpf_sepc = 0;
volatile uint64 g_timer_irq_count = 0;

volatile int g_irq_measure_on = 0;
volatile uint64 g_irq_measure_start_cycle = 0;

volatile uint64 g_irq_samples = 0;
volatile uint64 g_irq_sum_cycles = 0;
volatile uint64 g_irq_min_cycles = (uint64)-1;
volatile uint64 g_irq_max_cycles = 0;



extern void syscall(void);
extern void yield(void);
extern void panic(char *s);
//extern void printf(char *fmt, ...);

#define TIMER_INTERVAL 100000ULL

#define SCAUSE_INTERRUPT (1ULL << 63)
#define SCAUSE_CODE_MASK 0xfffULL

void timer_init(void)
{
  w_sie(r_sie() | SIE_STIE);  //允许S模式定时器开关

  // 首次安排：当前时间 + interval
  uint64 now = r_time();
  w_stimecmp(now + TIMER_INTERVAL_CYCLES);
  printf("timer_init: time=%p stimecmp=%p sie=%p sstatus=%p\n",
       r_time(), r_stimecmp(), r_sie(), r_sstatus());

}


void timer_interrupt(void)
{
  // 重新安排下一次 timer interrupt
  uint64 now_time = r_time();
  w_stimecmp(now_time + TIMER_INTERVAL_CYCLES);

  // 计数
  g_timer_irq_count++;  //中断次数

  // 性能统计：从 g_irq_measure_start_cycle 到现在的周期数
  if (g_irq_measure_on) {   //1开始统计
    uint64 now_cycle = rdcycle();  //读计数器
    uint64 delta = now_cycle - g_irq_measure_start_cycle; //减上一次
    g_irq_sum_cycles += delta;
    if (delta < g_irq_min_cycles) g_irq_min_cycles = delta;   //最短间隔
    if (delta > g_irq_max_cycles) g_irq_max_cycles = delta;
    g_irq_samples++;

    // 下一次从当前cycle开始
    g_irq_measure_start_cycle = now_cycle;
  }
    if (timer_interrupt_hook) {
    (*timer_interrupt_hook)++;
  }

}


uint64 handle_exception(uint64 scause, uint64 sepc, uint64 stval)
{
  extern volatile int g_illegal_seen;
  extern volatile int g_loadpf_seen;
  extern volatile uint64 g_expected_loadpf_sepc;

  if (scause == 8) {   //系统调用
    syscall();
    return sepc + 4;
  }

  if (scause == 2) {   //非法指令
    g_illegal_seen = 1;

    uint32 *p = (uint32 *)sepc;
    printf("Illegal: sepc=%p inst=%p next=%p stval=%p\n",
           sepc, (uint64)p[0], (uint64)p[1], stval);

    static int once = 0;
    if (once++ == 0)
      printf("Illegal: sepc=%p -> %p stval=%p\n", sepc, sepc + 4, stval);

    return sepc + 4;
  }

  if (scause == 3) {  //断点
    printf("Breakpoint: epc=%p stval=%p\n", sepc, stval);
    return sepc + 4;
  }

  if (scause == 12 || scause == 13 || scause == 15) {  //缺页
    printf("Page Fault: scause=%p epc=%p stval=%p\n", scause, sepc, stval);

    if (scause == 13 && sepc == g_expected_loadpf_sepc) {
      g_loadpf_seen = 1;
      return sepc + 4;
    }

    panic("unexpected page fault");
  }

  printf("Unknown Exception: scause=%p epc=%p stval=%p\n", scause, sepc, stval);
  panic("kerneltrap: unknown exception");
  return sepc;
}





// kernel/trap.c -> kerneltrap

void kerneltrap(void) {
    uint64 sepc = r_sepc();
    uint64 sstatus = r_sstatus();
    uint64 scause = r_scause();
    uint64 stval = r_stval();

    if ((sstatus & SSTATUS_SPP) == 0) {
        panic("kerneltrap: not from supervisor mode");
    }

    if (intr_get() != 0) {
        panic("kerneltrap: interrupts enabled");
    }

    // 处理中断
    if (scause & SCAUSE_INTERRUPT) {
        uint64 irq = scause & SCAUSE_CODE_MASK;

        switch (irq) {
            case 1: // Supervisor software interrupt
                w_sip(r_sip() & ~2);
                break;
            
            case 5: // Supervisor timer interrupt
                timer_interrupt(); // 现有的定时器处理
                
                // [新增] 只有在这里更新 ticks
                if (myproc() != 0 && myproc()->state == RUNNING) {
                    myproc()->ticks++; 
                    yield(); // 主动让出 CPU
                }
                break;
            
            case 9: // Supervisor external interrupt
                interrupt_dispatcher();
                break;
            
            default:
                printf("Unknown interrupt: %p scause=%p", irq, scause);
                break;
        }
    } else {
        // 处理异常
        uint64 new_sepc = handle_exception(scause, sepc, stval);
        w_sepc(new_sepc);
        w_sstatus(sstatus);
        return;
    }

    w_sepc(sepc);
    w_sstatus(sstatus);
}


