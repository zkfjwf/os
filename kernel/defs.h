// kernel/defs.h
struct spinlock;
struct sleeplock;

// kalloc.c
void*           alloc_page(void);
void            free_page(void *);
void            pmm_init(void);

// printf.c
void            panic(char*) __attribute__((noreturn));
int             printf(const char*, ...);

// string.c（如果你没有 string.c，就保留 kalloc.c 的 memset 并在别处提供 safestrcpy）
void*           memset(void *dst, int c, uint n);
//char*           safestrcpy(char *s, const char *t, int n);

// spinlock.c
void            acquire(struct spinlock*);
int             holding(struct spinlock*);
void            initlock(struct spinlock*, char*);
void            release(struct spinlock*);
void            push_off(void);
void            pop_off(void);

// riscv.h（intr_* 通常是 static inline，在头里就行；如果不是 inline，就要声明）
//void            intr_on(void);
//int             intr_get(void);

// uart.c
void            uartinit(void);
void            uart_puts(const char* s);
void            uart_putc(char c);
int             uartgetc(void);

// vm.c
int             map_page(uint64* pt, uint64 va, uint64 pa, int perm);
uint64*         walk_lookup(uint64* pt, uint64 va);
void            kvminit(void);
void            kvminithart(void);
int             copyout(uint64* pagetable, uint64 dstva, char *src, uint64 len);

// proc.c
struct proc*    myproc(void);
struct cpu*     mycpu(void);
void            procinit(void);
int             create_process(void (*entry)(void));
void            scheduler(void);
void            yield(void);
void            sleep(void *chan, struct spinlock *lk);
void            wakeup(void *chan);
void            exit_process(int status);
int             wait_process(int *status);
void sleep_ticks(int ticks);
// proc.c
int             setpriority(int, int);

#ifndef __KASSERT_H__
#define __KASSERT_H__

void panic(char *msg);
int             argaddr(int, uint64 *);

void shared_buffer_init(void);
void producer_task(void);
void consumer_task(void);

#define assert(x) \
    if(!(x)) panic("assertion failed: " #x);

#endif

