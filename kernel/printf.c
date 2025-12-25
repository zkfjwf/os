#include <stdarg.h>
#include "types.h"
#include "printf.h"
#include "riscv.h"
#include "defs.h"
struct spinlock{
    unsigned int locked;
    char *name;
    struct cpu *cpu;
};
static struct{
    struct spinlock lock;
}pr;
void pr_init()
{
    initlock(&pr.lock,"pr");
}
volatile int panicking = 0; // printing a panic message
volatile int panicked = 0; 
void
panic(char *s)
{
  panicking = 1;
  printf("panic: ");
  printf("%s\n", s);
  panicked = 1; // freeze uart output from other CPUs
  for(;;)
    ;
}
static char digits[] = "0123456789abcdef";
static void print_number(int num, int base, int sign)
{
   char buf[11];
   int i=0;
   uint32 x;
   if(sign&&num<0)
     x=-num;
    else
     x=num;
    sign=(num<0);
    do{
      buf[i++]=digits[x%base];
    }while((x/=base)!=0);
    if(sign)
    buf[i++]='-';
    while(--i>=0)
    {
        console_putc(buf[i]);
    }
}

static void
printptr(uint64 x)
{
  int i;
  console_putc('0');
  console_putc('x');
  for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
    console_putc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console.
int
printf(const char *fmt, ...)
{
  va_list ap;
  int i, cx, c0;
  char *s;

  acquire(&pr.lock);

  va_start(ap, fmt);
  for(i = 0; (cx = fmt[i] & 0xff) != 0; i++){
    if(cx != '%'){
      console_putc(cx);
      continue;
    }
    i++;
    c0 = fmt[i+0] & 0xff;
    if(c0 == 'd'){
      print_number(va_arg(ap, int), 10, 1);
    } else if(c0 == 'u'){
      print_number(va_arg(ap, uint32), 10, 0);
    }  else if(c0 == 'x'){
      print_number(va_arg(ap, uint32), 16, 0);
    } else if(c0 == 'p'){
      printptr(va_arg(ap, uint64));
    } else if(c0 == 'c'){
      console_putc(va_arg(ap, uint));
    } else if(c0 == 's'){
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        console_putc(*s);
    } else if(c0 == '%'){
      console_putc('%');
    } else if(c0 == 0){
      break;
    } else {
      // Print unknown % sequence to draw attention.
      console_putc('%');
      console_putc(c0);
    }

  }
  va_end(ap);

  release(&pr.lock);

  return 0;
}

int sprintf(char *buf, const char *fmt, ...)
{
  va_list ap;
  int i, cx, c0;
  int j=0;
  char *s;
  va_start(ap, fmt);
  for(i = 0; (cx = fmt[i] & 0xff) != 0; i++){
    if(cx != '%'){
      buf[j++]=cx;
      continue;
    }
    char tmp[11];
    int k=0;
    i++;
    c0 = fmt[i+0] & 0xff;
    if(c0 == 'd'){
      int x=va_arg(ap, int);
      uint xx;
      int sign=(x<0);
      if(sign)
        xx=-x;
      else
        xx=x;
      do{
        tmp[k++]=digits[xx%10];
      }while((xx/=10)!=0);
      if(sign)
        {tmp[k++]='-';}
       for(; k>=1; k--)
        buf[j++]=tmp[k-1];
    } else if(c0 == 'u'){
      int x=va_arg(ap, uint);
      do{
        tmp[k++]=digits[x%10];
      }while((x/=10)!=0);
        for(; k>=1; k--)
        buf[j++]=tmp[k-1];
    }  else if(c0 == 'x'){
      int x=va_arg(ap, uint);
      do{
        tmp[k++]=digits[x%16];
      }while((x/=16)!=0);
        for(; k>=1; k--)
        buf[j++]=tmp[k-1];
    } else if(c0 == 'c'){
      buf[j++]=va_arg(ap,uint);
    } else if(c0 == 's'){
      if((s = va_arg(ap, char*)) == 0)
        s = "(null)";
      for(; *s; s++)
        buf[j++]=*s;
    } else if(c0 == '%'){
      buf[j++]='%';
    } else if(c0 == 0){
      break;
    } else {
      // Print unknown % sequence to draw attention.
      buf[j++]='%';
      buf[j++]=c0;
    }

  }
  va_end(ap);

  //release(&pr.lock);

  return 0;
}
void clear_screen(void) {
  printf("\033[2J\033[H");
}
void clear_line(void) {
    printf("\033[K\r");
}
void goto_xy(int x,int y)
{
    printf("\033[H");
    x=x-1;
    y=y-1;
    while(x--)
    {
        printf("\n");
    }
    while(y--)
    {
       printf("\x1b[C"); 
    }
}



