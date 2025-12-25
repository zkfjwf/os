
#include "types.h"
#include "spinlock.h"
#include "defs.h"
#include "proc.h"

static struct spinlock buf_lock;
static int buf_full;
static int buf_item;

void shared_buffer_init(void)
{
  initlock(&buf_lock, "buf");
  buf_full = 0;   //是否缓冲区为空
  buf_item = 0;  //物品
}

void producer_task(void)
{
  for (int x = 1; x <= 5; x++) {
    acquire(&buf_lock);
    while (buf_full) {
      sleep(&buf_full, &buf_lock);
    }
    buf_item = x;
    buf_full = 1;
    wakeup(&buf_full);
    release(&buf_lock);
    yield();
  }
  exit_process(0);
}

void consumer_task(void)
{
  int sum = 0;
  for (int i = 0; i < 5; i++) {
    acquire(&buf_lock);
    while (!buf_full) {
      sleep(&buf_full, &buf_lock);
    }
    sum += buf_item;
    buf_full = 0;
    wakeup(&buf_full);
    release(&buf_lock);
    yield();
  }
  printf("consumer sum=%d\n", sum);
  exit_process(0);
}
