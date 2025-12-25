#include "kernel/types.h"
#include "user/user.h"

// 必须与内核中定义的结构体保持一致！
struct pstat {
    int pid;
    enum { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE } state;
    int priority;
    int ticks;
    // int wait_ticks; // 如果你的内核里有这个字段
    char name[16];
};

int main(int argc, char *argv[]) {
  struct pstat st[64]; // 假设最大进程数 NPROC 是 64
  int i;
  int count = getpinfo(&st); // 调用系统调用，假设它返回进程数量或总是成功

  printf("PID	PRIORITY	STATE		TICKS	NAME\n");
  for(i = 0; i < 64; i++) {
    if (st[i].state != UNUSED) {
        char *state_str;
        switch(st[i].state) {
            case SLEEPING: state_str = "SLEEPING"; break;
            case RUNNABLE: state_str = "RUNNABLE"; break;
            case RUNNING:  state_str = "RUNNING "; break;
            case ZOMBIE:   state_str = "ZOMBIE  "; break;
            default:       state_str = "UNKNOWN "; break;
        }

        printf("%d	%d		%s	%d	%s\n", 
               st[i].pid, 
               st[i].priority, 
               state_str, 
               st[i].ticks,
               st[i].name);
    }
  }
  exit(0);
}

