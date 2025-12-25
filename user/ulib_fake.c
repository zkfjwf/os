
#include "kernel/types.h"
#include "kernel/syscall.h"

// 声明 write 系统调用，不需要 include user.h，防止冲突
int write(int, const void*, int);

// 简易 strlen
int strlen(const char *s) {
  int n = 0;
  for(n = 0; s[n]; n++);
  return n;
}

// 简易 atoi
int atoi(const char *s) {
  int n;
  n = 0;
  while('0' <= *s && *s <= '9')
    n = n*10 + *s++ - '0';
  return n;
}

// 辅助函数：打印整数
static void printint(int xx, int base, int sign) {
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if(sign && (int)xx < 0) {
    x = -xx;
    write(1, "-", 1);
  } else
    x = xx;

  i = 0;
  do {
    buf[i++] = digits[x % base];
  } while((x /= base) != 0);

  while(--i >= 0)
    write(1, &buf[i], 1);
}

// 简易 printf (只支持 %d, %x, %s)
void printf(char *fmt, ...) {
  char *s;
  int c, i, state;
  uint *ap;

  state = 0;
  ap = (uint*)(void*)&fmt + 1;
  for(i = 0; fmt[i]; i++){
    c = fmt[i] & 0xff;
    if(state == 0){
      if(c == '%'){
        state = '%';
      } else {
        write(1, &c, 1);
      }
    } else if(state == '%'){
      if(c == 'd'){
        printint(*ap, 10, 1);
        ap++;
      } else if(c == 'x' || c == 'p'){
        printint(*ap, 16, 0);
        ap++;
      } else if(c == 's'){
        s = (char*)*ap;
        ap++;
        if(s == 0) s = "(null)";
        write(1, s, strlen(s));
      } else if(c == '%'){
        write(1, "%", 1);
      } else {
        write(1, "%", 1);
        write(1, &c, 1);
      }
      state = 0;
    }
  }
}
