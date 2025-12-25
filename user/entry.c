#include "kernel/types.h"
#include "user/user.h" // 确保引用了正确的头文件

int main(int argc, char *argv[]) {
  if(argc != 3){
    printf("Usage: nice pid priority\n");
    exit(0);
  }

  int pid = atoi(argv[1]);
  int prio = atoi(argv[2]);

  if (prio < 0 || prio > 20) { // 假设优先级范围是 0-20
      printf("Error: Priority must be between 0 and 20\n");
      exit(0);
  }

  printf("Setting pid %d to priority %d\n", pid, prio);
  setpriority(pid, prio);
  
  exit(0);
}
