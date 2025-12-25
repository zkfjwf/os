#include "types.h"
#include "spinlock.h"
#include "defs.h"
#define NPROC 64
#define MAX_PRIO 10
#define DEFAULT_PRIO 5
#define AGING_THRESHOLD 10 // 这里的单位是调度轮次
#define SYS_setpriority  22

// Saved registers for kernel context switches.
struct context {
  uint64 ra;
  uint64 sp;

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state.
struct cpu {
  struct proc *proc;          // The process running on this cpu, or null.
  struct context context;     // swtch() here to enter scheduler().
  int noff;                   // Depth of push_off() nesting.
  int intena;                 // Were interrupts enabled before push_off()?
};

struct cpu* mycpu(void);
extern struct cpu cpus[NCPU];


// trapframe kept for later (even if you暂时不用)
struct trapframe {
  /*   0 */ uint64 kernel_satp;
  /*   8 */ uint64 kernel_sp;
  /*  16 */ uint64 kernel_trap;
  /*  24 */ uint64 epc;
  /*  32 */ uint64 kernel_hartid;
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
};

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  struct spinlock lock;

  // p->lock must be held when using these:
  enum procstate state; //状态
  void *chan;  //等待通道
  int killed;  //是否被杀
  int xstate;  //推出状态
  int pid;  //进程id

  // wait_lock must be held when using this:
  struct proc *parent;

  // private to the process
  uint64 kstack;
  uint64 sz;
  uint64 *pagetable;
  struct trapframe *trapframe; //陷阱针完整保存用户态CPU现场
  struct context context;   //调度上下文
  struct inode *cwd;
  char name[16];
  int priority;       // 进程优先级 (0~10)
  int ticks;          // 已用 CPU 时间
  int wait_time;      // 等待时长（用于aging）
};

// APIs
struct proc* myproc(void);
extern struct proc proc[NPROC];
void procinit(void);

struct proc* alloc_process(void);
void free_process(struct proc *p);
int create_process(void (*entry)(void));
void exit_process(int status);
int wait_process(int *status);

void scheduler(void);
void yield(void);

void sleep(void *chan, struct spinlock *lk);
void wakeup(void *chan);

void swtch(struct context *old, struct context *new);

