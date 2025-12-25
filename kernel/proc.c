#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

#define NPROC 64
#define KSTACK_SIZE PGSIZE

struct cpu cpus[NCPU];

struct proc proc[NPROC];
static struct spinlock pid_lock;
static int nextpid = 1;

static int
allocpid(void)
{
  int pid;
  acquire(&pid_lock);
  pid = nextpid++;
  release(&pid_lock);
  return pid;
}

int
cpuid(void)
{
  // 单核，固定 0
  return 0;
}

struct cpu*
mycpu(void)
{
  return &cpus[cpuid()];
}

struct proc*
myproc(void)
{
  push_off();   //开关中断保护
  struct proc *p = mycpu()->proc;
  pop_off();
  return p;
}

void
procinit(void)
{
  initlock(&pid_lock, "pid");
  for(int i = 0; i < NPROC; i++){
    initlock(&proc[i].lock, "proc");
    proc[i].state = UNUSED;
    proc[i].kstack = 0;
    proc[i].pid = 0;
    proc[i].parent = 0;
    proc[i].chan = 0;
    proc[i].killed = 0;
    proc[i].xstate = 0;
    memset(&proc[i].context, 0, sizeof(proc[i].context));
  }

}

struct proc*
alloc_process(void)
{
  for(int i = 0; i < NPROC; i++){
    struct proc *p = &proc[i];
    acquire(&p->lock);
    if(p->state == UNUSED){
      p->state = USED;  //改为used
      p->pid = allocpid();
      p->killed = 0;
      p->xstate = 0;
      p->parent = 0;
      p->chan = 0;
    p->priority = DEFAULT_PRIO;
  p->ticks = 0;
  p->wait_time = 0;
      p->kstack = (uint64)alloc_page();
      if(p->kstack == 0){
        p->state = UNUSED;
        release(&p->lock);
        return 0;
      }

      memset(&p->context, 0, sizeof(p->context));
      p->name[0] = 'p';
p->name[1] = 'r';
p->name[2] = 'o';
p->name[3] = 'c';
p->name[4] = 0;


      release(&p->lock);
      return p;
    }
    release(&p->lock);
  }
  return 0;
}

void
free_process(struct proc *p)
{
  acquire(&p->lock);

  if(p->kstack){
    free_page((void*)p->kstack);
    p->kstack = 0;
  }

  p->pid = 0;
  p->parent = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;

  release(&p->lock);
}

int
create_process(void (*entry)(void))
{
  struct proc *p = alloc_process();
  if(p == 0) return -1;

  acquire(&p->lock);

  // 第一次被调度时，swtch 恢复 ra/sp，然后 ret -> entry()
  p->context.ra = (uint64)entry;  //从entry开始运行
  p->context.sp = p->kstack + KSTACK_SIZE;

  p->parent = myproc();
  p->state = RUNNABLE;

  int pid = p->pid;
  release(&p->lock);
  return pid;
}

static void
sched(void)
{
  struct proc *p = myproc();
  struct cpu *c = mycpu();

  if(p == 0) panic("sched: no proc");
  if(!holding(&p->lock)) panic("sched: p->lock not held");
  if(intr_get()) panic("sched: interrupts enabled");

  swtch(&p->context, &c->context);
}   //从当前进程切回调度器

void  //让出CPU
yield(void)
{
  struct proc *p = myproc();
  if(p == 0) return;

  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// kernel/proc.c -> scheduler()

// kernel/proc.c -> scheduler()

struct proc* 
select_highest_priority(void) 
{
  struct proc *p;
  struct proc *best = 0;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    
    if(p->state == RUNNABLE) {
      // 如果还没有 best，或者当前 p 的优先级高于 best
      if(!best || p->priority > best->priority) {
        // 如果之前已经有一个 best，释放它的锁
        if(best) release(&best->lock);
        
        best = p;
        continue; // 继续循环，保持持有 best 的锁
      }
    }
    
    release(&p->lock);
  }
  return best;
}

// [修改] 调度器主循环
// 定义在 kernel/proc.c 中

// 用于记录上次调度到的位置，实现 RR
struct proc *last_run_proc = 0; 

void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  
  c->proc = 0;
  for(;;){
    intr_on(); // 开启中断

    int found_proc = 0;
    int max_prio = -1;

    // ----------------------------------------------------
    // 第一步：执行 Aging 并 找出当前全局最高的优先级
    // ----------------------------------------------------
    for(p = proc; p < &proc[NPROC]; p++){
      acquire(&p->lock);
      if(p->state == RUNNABLE){
        // Aging 逻辑
        p->wait_time++;
        if(p->wait_time > AGING_THRESHOLD){
          if(p->priority < MAX_PRIO){
            p->priority++;
            // printf("PID %d aged to %d\n", p->pid, p->priority);
          }
          p->wait_time = 0;
        }

        // 记录系统中的最高优先级
        if(p->priority > max_prio) {
            max_prio = p->priority;
        }
      }
      release(&p->lock);
    }

    // 如果没有任何 RUNNABLE 进程，max_prio 仍为 -1，空转等待
    if(max_prio == -1) {
       continue;
    }

    // ----------------------------------------------------
    // 第二步：执行 Round-Robin 调度 (仅针对最高优先级的进程)
    // ----------------------------------------------------
    
    // 我们需要遍历所有进程找到 priority == max_prio 的那个。
    // 为了实现 RR，我们不能总是从 proc[0] 开始找。
    // 我们从 last_run_proc 的下一个位置开始找，绕一圈回来。
    
    struct proc *start_p;
    if (last_run_proc != 0 && last_run_proc >= proc && last_run_proc < &proc[NPROC]) {
        start_p = last_run_proc + 1;
    } else {
        start_p = proc;
    }

    // 遍历一整圈
    for(int i = 0; i < NPROC; i++){
        p = start_p + i;
        // 如果指针越界了，绕回到数组头部
        if(p >= &proc[NPROC]) {
            p = proc + (p - &proc[NPROC]);
        }

        acquire(&p->lock);
        // 核心判断：只有状态是 RUNNABLE 且 优先级等于最高优先级 时才运行
        if(p->state == RUNNABLE && p->priority == max_prio){
            
            // 找到了！运行它
            p->state = RUNNING;
            p->wait_time = 0; 
            
            c->proc = p;
            
            // 记录这次运行的是谁，下次从它后面开始找
            last_run_proc = p; 

            swtch(&c->context, &p->context);
            
            // --- 进程 yield 后回到这里 ---
            c->proc = 0;
            found_proc = 1;
            release(&p->lock);
            
            // 这一轮只调度这一个进程，然后跳出循环重新计算 Aging 和 Max Prio
            // 这样能保证高优先级新来时能被及时感知
            break; 
        }
        release(&p->lock);
    }
  }
}


void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  if(p == 0) panic("sleep: no proc");
  if(lk == 0) panic("sleep: no lock");

  acquire(&p->lock);
  release(lk);

  p->chan = chan;   //等待
  p->state = SLEEPING;
  sched();   //切换到调度器

  p->chan = 0;
  release(&p->lock);

  acquire(lk);
}

void
wakeup(void *chan)
{
  for(int i = 0; i < NPROC; i++){
    struct proc *p = &proc[i];
    acquire(&p->lock);
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
    }
    release(&p->lock);
  }
}

void
exit_process(int status)
{
  struct proc *p = myproc();
  if(p == 0) panic("exit: no proc");

  acquire(&p->lock);
  p->xstate = status;
  p->state = ZOMBIE;  //状态为ZOMBIE
  release(&p->lock);

  if(p->parent)
    wakeup(p->parent);  //叫父进程

  acquire(&p->lock);
  sched();
  panic("exit returned");
}

int
wait_process(int *status)
{
  struct proc *p = myproc();
  if(p == 0) return -1;

  for(;;){
    int havekids = 0;

    for(int i = 0; i < NPROC; i++){
      struct proc *pp = &proc[i];
      acquire(&pp->lock);
      if(pp->parent == p){
        havekids = 1;
        if(pp->state == ZOMBIE){  //状态为ZOMBIE
          int pid = pp->pid;
          if(status) *status = pp->xstate;
          release(&pp->lock);
          free_process(pp);  //回收子进程
          return pid;
        }
      }
      release(&pp->lock);
    }

    if(!havekids)
       return -1;

    acquire(&p->lock);
    sleep(p, &p->lock);
    release(&p->lock);
  }
}

void sleep_ticks(int ticks)
{
  uint64 start = get_time();
  while (get_time() - start < (uint64)ticks) {
    yield();
  }
}

// kernel/proc.c

// kernel/proc.c

int
setpriority(int pid, int value)
{
  struct proc *p;
  if(value < 0 || value > MAX_PRIO) return -1;

  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      p->priority = value;
      p->wait_time = 0; // 优先级改变，重置等待时间
      release(&p->lock);
      return 0; // 成功
    }
    release(&p->lock);
  }
  return -1; // 未找到
}

int
getpriority(int pid)
{
  struct proc *p;
  int prio = -1;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid){
      prio = p->priority;
      release(&p->lock);
      return prio;
    }
    release(&p->lock);
  }
  return prio;
}
// kernel/proc.c

// kernel/proc.c 底部追加

// --- [新增] 本地简易字符串函数，防止编译报错 ---
void
my_strcpy(char *dst, const char *src, int n)
{
  int i;
  for(i = 0; i < n-1 && src[i] != 0; i++){
    dst[i] = src[i];
  }
  dst[i] = 0;
}

int
my_strncmp(const char *p, const char *q, int n)
{
  while(n > 0 && *p && *p == *q)
    n--, p++, q++;
  if(n == 0)
    return 0;
  return (uchar)*p - (uchar)*q;
}
// kernel/proc.c

// [新增] 线程入口包装器
void
kthread_wrapper(void)
{
  struct proc *p = myproc();
  
  // 1. 关键：释放调度器持有的进程锁
  release(&p->lock);
  
  // 2. 从暂存位置取出真实的函数指针
  void (*func)(void) = (void (*)(void))p->trapframe;
  
  // 3. 执行真实任务
  if(func) func();
  
  // 4. 任务结束，自行了断
  exit_process(0);
}

// --- [修改] 通用内核线程创建 ---
// 1. 使用 alloc_page() 替代 kalloc()
// 2. 使用 my_strcpy 替代 safestrcpy
// kernel/proc.c 中的 create_kthread 函数

void
create_kthread(char *name, int prio, void (*func)(void))
{
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->state == UNUSED) goto found;
    release(&p->lock);
  }
  return;

found:
  p->pid = allocpid();
  p->state = USED;
  p->priority = prio;
  p->ticks = 0;
  p->wait_time = 0;
  
  // 使用之前定义的 my_strcpy
  my_strcpy(p->name, name, sizeof(p->name));

  // 分配内核栈
  void *kstack = alloc_page();
  if(!kstack) panic("kthread alloc_page");
  p->kstack = (uint64)kstack;

  // --- [修改部分开始] ---
  
  // 1. 借用 trapframe 指针来传递函数地址 (Hack but safe for kthreads)
  p->trapframe = (struct trapframe *)func;

  // 2. 设置上下文，让线程启动时先跳到 kthread_wrapper
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)kthread_wrapper; 
  p->context.sp = p->kstack + KSTACK_SIZE;

  // --- [修改部分结束] ---

  p->state = RUNNABLE;
  release(&p->lock);
  
  // 注意：这里是在 release 之后打印，是安全的
  printf("Kernel Thread Created: %s (PID %d, PRIO %d)\n", name, p->pid, prio);
}

// --- [不变] CPU 消耗任务 ---
void
cpu_bound_task(void)
{
  struct proc *p = myproc();
  for(;;) {
    volatile int i;
    for(i = 0; i < 1000000; i++); 

    if(p->ticks % 20 == 0) {
      printf("[TASK] PID %d | PRIO %d | TICKS %d | WAIT %d\n", 
             p->pid, p->priority, p->ticks, p->wait_time);
    }
  }
}

// --- [修改] 封装接口供 Monitor 使用 ---
void create_test_task(char *name, int prio) {
    create_kthread(name, prio, cpu_bound_task);
}
// 定义一个纯计算任务，不主动让出 CPU
void task_cpu_bound(void) {
  while(1) {
    // 纯计算，死循环
    // 只有时钟中断能打断我！
    for(volatile int i=0; i<1000000; i++);
  }
}

// kernel/proc.c

// 在 kernel/proc.c 或你的测试文件里

// 在你的 monitor 代码所在的文件中

void kthread_exit() {
    struct proc *p = myproc();
    printf(">>> Monitor Thread Exiting (Self-Destruct)...\n");
    
    // 1. 必须先获取锁！
    acquire(&p->lock); 
    
    // 2. 修改状态，让自己变成“死人”
    p->state = UNUSED; 
    
    // 3. 直接调度。注意：千万不要在这里调用 release(&p->lock)！
    // sched() 会切换到调度器线程，调度器线程会帮你释放这把锁。
    sched();
    
    // 4. 永远不会执行到这里
    panic("zombie exit");
}

/*
void monitor_task(void) {
    int tick_count = 0;
    
    // 先创建两个测试线程（如果你之前是在 start 里创建的，可以移到这里，或者保持原样）
    // 假设 PID 2 是 HIGH, PID 3 是 LOW
    create_kthread("HIGH_T1", 10, task_high); // 确保 task_high 是死循环
    create_kthread("LOW_T1", 0, task_low);    // 确保 task_low 是死循环

    printf("Monitor: Task started! Observing for 15 ticks...\n");

    while (1) {
        tick_count++;

        // 1. 打印表头和当前进程状态
        printf("--- Monitor Tick %d ---\n", tick_count);
        debug_proc_table(); // 打印你那张漂亮的表格

        // 2. 控制实验逻辑
        
        // [阶段 1] 前 5 次：保持高低优先级 (T1场景)
        if (tick_count < 6) {
            // 什么都不做，观察 Ticks 差异
            // 预期: HIGH 狂涨，LOW 不动
        }
        
        // [阶段 2] 第 6 次：切换为同优先级 (T2场景 - RR)
        else if (tick_count == 6) {
            printf(">>> [SWITCH] Setting both priorities to 5 (Round Robin)\n");
            // 这里你需要一种方式设置优先级，如果在内核态，直接修改 proc 数组即可
            // 假设 PID 2 和 3 是固定的，或者你需要遍历查找
            struct proc *p;
            for(p = proc; p < &proc[NPROC]; p++){
                if(p->state != UNUSED && p->pid >= 2) { 
                    acquire(&p->lock);
                    p->priority = 5;
                    printf("DEBUG: PID %d priority set to 5\n", p->pid);
                    release(&p->lock);
                }
            }
        }

        // [阶段 3] 观察 RR 效果后退出
        else if (tick_count >= 15) {
            printf(">>> Experiment Finished. Monitor is leaving to let tasks run freely.\n");
            kthread_exit(); // <--- 关键：在这里自杀！
        }

        // 3. 关键：睡久一点！
        // 之前可能睡太短，导致 Monitor 一直抢占
        // 现在睡 10 个 tick，意味着这 10 个 tick 全归 HIGH/LOW 跑
        sleep_ticks(10); 
    }
}
*/