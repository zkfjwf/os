
#include "pstat.h" // 记得引入头文件
#include "proc.h"

extern struct proc proc[NPROC]; // 引用进程表

// 设置优先级的系统调用
uint64
sys_setpriority(void)
{
  int pid, priority;
  struct proc *p;

  // 获取两个参数: pid 和 priority
  if(argint(0, &pid) < 0 || argint(1, &priority) < 0)
    return -1;

  // 简单的边界检查 (假设优先级范围 0-10)
  if(priority < 0 || priority > 10) return -2;

  // 遍历进程表寻找 PID
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    if(p->pid == pid && p->state != UNUSED){
      p->priority = priority;
      p->wait_time = 0; // 修改优先级后重置等待时间，防止立刻被 aging
      release(&p->lock);
      return 0; // 成功
    }
    release(&p->lock);
  }
  return -1; // 没找到 PID
}
// Like strncpy but guaranteed to NUL-terminate.
char*
safestrcpy(char *s, const char *t, int n)
{
  char *os;

  os = s;
  if(n <= 0)
    return os;
  while(--n > 0 && (*s++ = *t++) != 0)
    ;
  *s = 0;
  return os;
}
// 获取所有进程信息的系统调用 (给 ps 用)
uint64
sys_getpinfo(void)
{
  uint64 addr; // 用户态传入的结构体指针地址
  struct pstat kst; // 内核态的临时结构体
  struct proc *p;
  
  if(argaddr(0, &addr) < 0)
    return -1;

  // 填充数据
  int i = 0;
  for(p = proc; p < &proc[NPROC]; p++, i++){
    acquire(&p->lock);
    kst.inuse[i] = (p->state != UNUSED);
    if(p->state != UNUSED){
      kst.pid[i] = p->pid;
      kst.priority[i] = p->priority;
      kst.ticks[i] = p->ticks;
      kst.state[i] = p->state;
      kst.wait_time[i] = p->wait_time;
      safestrcpy(kst.name[i], p->name, sizeof(p->name));
    }
    release(&p->lock);
  }

  // 将数据从内核空间复制到用户空间
  if(copyout(myproc()->pagetable, addr, (char *)&kst, sizeof(kst)) < 0)
    return -1;
  
  return 0;
}
