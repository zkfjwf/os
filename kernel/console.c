#include <stdarg.h>
//#define BACKSPACE 0x100
void console_putc(int c)
{
    if(c==8||c==127){
    uart_putc('\b'); uart_putc(' '); uart_putc('\b');  //先回退再写一个空格再回退
  } else {
    uart_putc(c);
  }
}
void console_puts(const char* s)
{
    while(*s){
    console_putc(*s++);
  }
}