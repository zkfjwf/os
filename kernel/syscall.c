
#include "types.h"
#include "proc.h"
#include "riscv.h"
#include "defs.h"
#include "syscall.h"
//extern void printf(char *fmt, ...);

// 系统调用入口
void syscall(void) {
    // 标准实现：
    // struct proc *p = myproc();
    // int num = p->trapframe->a7;
    // ... 根据 num 调用对应的 sys_write, sys_fork 等 ...
    
    printf("syscall: handled");
}
// kernel/syscall.c
static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  *ip = argraw(n);
  return 0;
}
// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}



extern uint64 sys_getpinfo(void);

static uint64 (*syscalls[])(void) = {
// ... 原有的 ...

[SYS_getpinfo]    sys_getpinfo,
};
