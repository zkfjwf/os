#define NCPU 8
#include"types.h"
#include"proc.h"
#define NPROC 64 
#include"riscv.h"
#include"defs.h"
extern void trap_init(void);
extern void timer_init(void);
extern void clear_screen(void);
extern void console_puts(char *s);
volatile uint64 g_backup_ra;
volatile uint64 g_backup_sp;

extern void pr_init(void); 

extern volatile int *timer_interrupt_hook;
// kernel/main.c

// 1. 声明外部函数
extern void create_kthread(char *name, int prio, void (*func)(void));
extern void monitor_task(void);

void main()
{
}

void test_printf_basic() {
printf("Testing integer: %d\n", 42);
printf("Testing negative: %d\n", -123);
printf("Testing zero: %d\n", 0);
printf("Testing hex: 0x%x\n", 0xABC);
printf("Testing string: %s\n","Hello");
printf("Testing char: %c\n",'X');
printf("Testing percent: %%\n");
printf("INT_MAX: %d\n", 2147483647);
printf("INT_MIN: %d\n", -2147483648);
printf("NULL string: %s\n", (char*)0);
printf("Empty string: %s\n","");

}
__attribute__ ((aligned (16))) char stack0[4096 * NCPU];
void test_physical_memory(void) {
// 测试基本分配和释放
void *page1 = alloc_page();
void *page2 = alloc_page();
assert(page1 != page2);
assert(((uint64)page1 & 0xFFF) == 0); // 页对齐检查
// 测试数据写入
*(int*)page1 = 0x12345678;
assert(*(int*)page1 == 0x12345678);
// 测试释放和重新分配
free_page(page1);
void *page3 = alloc_page();
// page3可能等于page1（取决于分配策略）
free_page(page2);
free_page(page3);
printf("physical_memory alloc access\n");
}

void test_pagetable(void) {
pagetable_t pt = alloc_page();
  memset(pt, 0, PGSIZE); 
// 测试基本映射
uint64 va = 0x1000000;
uint64 pa = (uint64)alloc_page();
assert(map_page(pt, va, pa, PTE_R | PTE_W) == 0);
// 测试地址转换
pte_t *pte = walk_lookup(pt, va);
assert(pte != 0 && (*pte & PTE_V));
assert(PTE_PA(*pte) == pa);
// 测试权限位
assert(*pte & PTE_R);
assert(*pte & PTE_W);
assert(!(*pte & PTE_X));
printf("pagetable test access\n");
} 
void test_virtual_memory(void)
{
  // ==================== 分页启用前 ====================
  printf("Before enabling paging...\n");

  // -------- 测试 1：内核数据访问（分页前） --------
  static int test_data = 12345;
  printf("Data before paging: %d\n", test_data);

  // -------- 测试 2：简单计算（验证代码可执行） --------
  int x = 10;
  int y = 20;
  printf("Compute before paging: %d\n", x + y);

  // ==================== 启用分页 ====================
  printf("Enabling paging...\n");

  kvminit();       // 构建内核页表
  kvminithart();   // 写 satp，开启 Sv39 分页

  // ==================== 分页启用后 ====================
  printf("After enabling paging...\n");

  // -------- 测试 3：内核代码是否仍可执行 --------
  int a = 7;
  int b = 8;
  printf("Compute after paging: %d\n", a * b);

  // -------- 测试 4：内核数据是否仍可访问 --------
  test_data += 1;
  printf("Data after paging: %d\n", test_data);

  // -------- 测试 5：栈是否仍然可用 --------
  int stack_test = 0xdead;
  printf("Stack test value: 0x%x\n", stack_test);

  // -------- 测试 6：设备访问是否正常（UART） --------
  printf("UART still works after paging.\n");

  // ==================== 结束标记 ====================
  printf("Virtual memory test finished successfully.\n");

  // 防止函数返回导致未知行为
}

void test_timer_interrupt(void) {
printf("Testing timer interrupt...\n");
// 记录中断前的时间
uint64 start_time = get_time();
volatile int interrupt_count = 0;
// 设置测试标志
timer_interrupt_hook = &interrupt_count;
// 在时钟中断处理函数中增加计数
// 等待几次中断

while (interrupt_count < 5) {
static int last_count = -1;
        if (interrupt_count != last_count) {
             printf("Waiting for interrupt %d...", interrupt_count + 1);
             last_count = interrupt_count;
        }
// 简单延时
for (volatile int i = 0; i < 1000000; i++);
}
timer_interrupt_hook = 0;
uint64 end_time = get_time();
printf("Timer test completed: %d interrupts in %d cycles\n"
, interrupt_count, (int)end_time - start_time);
}
__attribute__((noinline))
void cause_illegal_instruction(void)
{
  __asm__ volatile(
    ".align 2\n"
    ".word 0xffffffff\n"   // illegal
    ".word 0x00008067\n"   // jalr x0, x1, 0   (return)
    :
    :
    : "memory"
  );
}

static inline void cause_load_access_fault(void)
{
  extern volatile uint64 g_expected_loadpf_sepc;
  extern volatile int g_loadpf_seen;

  g_loadpf_seen = 0;

  // 用内联汇编：
  // 1) 把下一条会执行的 "ld" 指令地址写入 g_expected_loadpf_sepc
  // 2) 执行一次对 0 地址的 load，必然触发 load page fault
  asm volatile(
      "la   t0, 1f\n"
      "sd   t0, 0(%0)\n"
      "1:\n"
      "ld   t1, 0(zero)\n"
      :
      : "r"(&g_expected_loadpf_sepc)
      : "t0", "t1", "memory");

  // 如果 handler 正确跳过了那条 ld，会回到这里
  while (g_loadpf_seen == 0)
    ;
}


void
test_exception_handling(void)
{
  kvminit();       // 构建内核页表
  kvminithart(); 
   //  asm volatile("mv %0, ra" : "=r" (g_backup_ra));
   // asm volatile("mv %0, sp" : "=r" (g_backup_sp));
  printf("Testing exception handling...\n");

  printf("[1/2] illegal instruction...\n");
  cause_illegal_instruction();
  printf("[1/2] returned after illegal instruction trap\n");

  printf("[2/2] load access fault...\n");
  cause_load_access_fault();
  printf("[2/2] returned after load fault trap\n");

  printf("Exception tests completed\n");
 /*asm volatile(
        "mv ra, %0\n"   
        "mv sp, %1\n"   
        "ret"            
        : 
        : "r" (g_backup_ra), "r" (g_backup_sp)
    );  */
    while(1);
}


void test_interrupt_overhead(void)
{
  printf("Testing interrupt overhead...\n");

  extern volatile uint64 g_timer_irq_count;
  extern volatile int g_irq_measure_on;
  extern volatile uint64 g_irq_measure_start_cycle;
  extern volatile uint64 g_irq_samples, g_irq_sum_cycles, g_irq_min_cycles, g_irq_max_cycles;

  // 等待下一次 timer interrupt，作为同步点
  uint64 base = g_timer_irq_count;
  while (g_timer_irq_count == base) { }
  g_irq_measure_start_cycle = rdcycle();

  // 清空统计
  g_irq_samples = 0;
  g_irq_sum_cycles = 0;
  g_irq_min_cycles = (uint64)-1;
  g_irq_max_cycles = 0;

  // 开始测量 N 次中断间隔的周期数（包含中断处理开销+回到内核继续跑的代价）
  const uint64 N = 100;
  uint64 start = g_timer_irq_count;

  g_irq_measure_on = 1;
  while (g_timer_irq_count - start < N) { }
  g_irq_measure_on = 0;

  if (g_irq_samples == 0) {
    printf("interrupt overhead: no samples\n");
    return;
  }

  uint64 avg = g_irq_sum_cycles / g_irq_samples;
  printf("interrupt overhead: samples=%d avg=%d cycles min=%d max=%d\n",
         (int)g_irq_samples, (int)avg, (int)g_irq_min_cycles, (int)g_irq_max_cycles);
}
static void simple_task(void)
{
  // 做一点事，确保真的运行过
  for (volatile int i = 0; i < 200000; i++) ;
  exit_process(0);
}
void test_process_creation(void) {
printf("Testing process creation...\n");
// 测试基本的进程创建
int pid = create_process(simple_task);
assert(pid > 0);
// 测试进程表限制
int pids[NPROC];
int count = 0;
for (int i = 0; i < NPROC + 5; i++) {
int pid = create_process(simple_task);
if (pid > 0) {
pids[count++] = pid;
} else {
break;
}
(void)pids;
}
printf("Created %d processes\n"
, count);
// 清理测试进程
for (int i = 0; i < count; i++) {
wait_process(0);
}
}
void cpu_intensive_task(void)
{
  for (volatile int i = 0; i < 5000000; i++) ;
  exit_process(0);
}


void test_scheduler(void) {
printf("Testing scheduler...\n");
// 创建多个计算密集型进程
for (int i = 0; i < 3; i++) {
create_process(cpu_intensive_task);
}
// 观察调度行为
uint64 start_time = get_time();
sleep_ticks(1000);

uint64 end_time = get_time();
printf("Scheduler test completed in %d cycles\n"
,
end_time - start_time);
}
void test_synchronization(void) {
// 测试生产者-消费者场景
shared_buffer_init();
create_process(producer_task);
create_process(consumer_task);
// 等待完成
wait_process(0);
wait_process(0);
printf("Synchronization test completed\n");
}
// 定义状态字符串数组，方便把 3, 4 转换成文字
static char *states[] = {
  [UNUSED]    "UNUSED  ",
  [USED]      "USED    ",
  [SLEEPING]  "SLEEPING",
  [RUNNABLE]  "RUNNABLE",
  [RUNNING]   "RUNNING ",
  [ZOMBIE]    "ZOMBIE  "
};

void debug_proc_table(void) {
    struct proc *p;
    printf("PID	NAME		STATE		PRIO	TICKS\n");
    printf("----------------------------------------------------\n");
    
    for(p = proc; p < &proc[NPROC]; p++){
        if(p->state == UNUSED) continue;
        
        // 打印基本信息
        // 注意：p->state 可能越界，加个保护
        char *state_str = (p->state >= 0 && p->state < 6) ? states[p->state] : "UNKNOWN ";

        printf("%d	%s		%s	%d	%d\n", 
            p->pid, 
            p->name, 
            state_str, 
            p->priority, 
            p->ticks
        );
    }
    printf("----------------------------------------------------\n");
}

volatile static int started = 0;


// --- 延时 ---
void delay_loop(int n) {
  volatile int i;
  for(i = 0; i < n * 10000; i++);
}

// --- 测试任务 ---

void task_high() {
  while(1) {
    printf("[HIGH] Running (Prio %d)\n", myproc()->priority);
    delay_loop(50);
    yield(); // 模拟时间片耗尽
  }
}

void task_mid() {
  while(1) {
    printf("  [MID] Running (Prio %d)\n", myproc()->priority);
    delay_loop(50);
    yield();
  }
}

void task_low() {
  while(1) {
    printf("    [LOW] Running (Prio %d)\n", myproc()->priority);
    delay_loop(50);
    yield();
  }
}
//main.c（或你的 start.c 里调用）
void start(void){
   w_tp(0);
  uartinit();
    trap_init();   // 初始化中断向量表
    timer_init();  // 
    intr_on();     // 开启全局中断
  clear_screen();
  pr_init();
  console_puts("Hello OS\n");
  //clear_line();
  pmm_init();
  procinit();
  //test_printf_basic();
  //test_physical_memory();
  //test_pagetable();
  //test_virtual_memory();
  //test_timer_interrupt();
  //test_exception_handling();
  //test_interrupt_overhead();
  //test_process_creation();
  //test_scheduler();
  //test_synchronization();
  //goto_xy(3,5);   //移动光标到第三行第五列


/*
  for(;;){
    int ch = uartgetc();
    if(ch >= 0) console_putc((char)ch);
  }
  
 */

  printf(">>> Scheduler Test: HIGH(10) vs LOW(1) <<<\n");

  // 直接创建两个死循环任务
  create_kthread("HIGH", 10, task_high);
  create_kthread("LOW", 1,  task_low);

  printf(">>> Entering Scheduler...\n");
  scheduler(); // 启动调度
  
  panic("scheduler returned");
  
}

