#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int pid = getpid();
  printf("Loop process %d started...\n", pid);
  
  // 死循环消耗 CPU
  while(1) {
      // 空循环，或者做简单的数学运算防止被编译器优化掉
      volatile int x = 0;
      x = x + 1;
  }
  exit(0);
}
