#include "types.h"
#include "riscv.h"
#include <stdint.h>
#include "defs.h"
#define UART0        0x10000000UL

// 寄存器偏移（按 16550 标准）
#define RHR 0                 // 接收寄存器
#define THR 0                 // 发送寄存器
#define IER 1                 // interrupt enable register
#define IER_RX_ENABLE (1<<0)
#define IER_TX_ENABLE (1<<1)
#define FCR 2                 // FIFO control register
#define FCR_FIFO_ENABLE (1<<0)
#define FCR_FIFO_CLEAR (3<<1) // clear the content of the two FIFOs
#define ISR 2                 // interrupt status register
#define LCR 3                 // line control register
#define LCR_EIGHT_BITS (3<<0)
#define LCR_BAUD_LATCH (1<<7) // special mode to set baud rate
#define LSR 5                 // 检查串口状态(是否可以接收发送)
#define LSR_RX_READY (1<<0)   // 有接收可以读  值为1
#define LSR_TX_IDLE (1<<5)    // THR空  00100000  

#define Reg(reg) ((volatile unsigned char *)(UART0 + (reg)))
#define ReadReg(reg) (*(Reg(reg)))
#define WriteReg(reg, v) (*(Reg(reg)) = (v))

// 初始化：38400 波特率，8N1，开 FIFO
void
uartinit(void)
{
  // disable interrupts.
  WriteReg(IER, 0x00);   //关中断

  // special mode to set baud rate.
  WriteReg(LCR, LCR_BAUD_LATCH);  //开始波特率设置

  // LSB for baud rate of 38.4K.
  WriteReg(0, 0x03);

  // MSB for baud rate of 38.4K.
  WriteReg(1, 0x00);

  // leave set-baud mode,
  // and set word length to 8 bits, no parity.
  WriteReg(LCR, LCR_EIGHT_BITS);    //关闭波特率设置

  // reset and enable FIFOs.
  WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);   //开FIFO并清空
}

// 同步发送一个字节：等 THR 空再写
int
uartgetc(void)
{
  if(ReadReg(LSR) & LSR_RX_READY){
    // input data is ready.
    return ReadReg(RHR);
  } else {
    return -1;
  }
}

void uart_putc(char c){
  while(((ReadReg(LSR)) & LSR_TX_IDLE) == 0) { /* busy-wait */ }
  WriteReg(THR,c);
}

// 同步发送字符串
void uart_puts(const char* s){
  while(*s){
    uart_putc(*s++);
  }
}


