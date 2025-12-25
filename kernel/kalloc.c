#include "types.h"
#include "proc.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

struct run {
  struct run *next;
};

extern char end[]; // first address after kernel. defined by kernel.ld.

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void*
memset(void *dst, int c, uint n)
{
  char *cdst = (char *) dst;
  for(uint i = 0; i < n; i++){
    cdst[i] = c;
  }
  return dst;
}

void
free_page(void* page)
{
  struct run *r;

  if(((uint64)page % PGSIZE) != 0 || (char*)page < end || (uint64)page >= PHYSTOP)
    panic("free_page");

  memset(page, 1, PGSIZE);

  r = (struct run*)page;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

static void
freerange(void *pa_start, void *pa_end)
{
  char *p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    free_page(p);
}

void
pmm_init(void)
{
  initlock(&kmem.lock,"kmem");
  freerange(end, (void*)PHYSTOP);
}

void*
alloc_page(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE);
  return (void*)r;
}
