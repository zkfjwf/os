#include"types.h"
#include"riscv.h"
#include "defs.h"

pagetable_t create_pagetable(void)
{
    pagetable_t pt;
    pt=(pagetable_t) alloc_page();
    if(pt==0)
        return 0;
    memset(pt,0,PGSIZE);
    return pt;
}
pagetable_t walk_create(pagetable_t pt, uint64 va)
{
    if(va >= MAXVA)
    panic("walk_create");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pt[VPN_MASK(va, level)];    //得到页表项的地址
    if(*pte & PTE_V) 
    {
      pt = (pagetable_t)PTE2PA(*pte);
    } else {
      if((pt = (pde_t*)alloc_page()) == 0)
        return 0;
      memset(pt, 0, PGSIZE);
      *pte = PA2PTE(pt) | PTE_V;
    }
  }
  return &pt[PX(0, va)];   //取地址的地址的原因是便于修改,在mappages
}
pte_t* walk_lookup(pagetable_t pt, uint64 va){
     if(va >= MAXVA)
    panic("walk_lookup");

  for(int level = 2; level > 0; level--) {
    pte_t *pte = &pt[PX(level, va)];    //得到页表项的地址
    if(*pte & PTE_V) 
    {
      pt = (pagetable_t)PTE2PA(*pte);
    } else {
      return 0;  //无此对应物理页
    }
  }
  return &pt[PX(0, va)];   //取地址的地址的原因是便于修改,在mappages
}
int map_page(pagetable_t pt, uint64 va, uint64 pa, int perm)
{
  pagetable_t pte;

  if((va % PGSIZE) != 0)
    panic("mappages: va not aligned");

    if((pte = walk_create(pt, va)) == 0)
      return -1;
    if(*pte & PTE_V)
      panic("mappages: remap");
    *pte = PA2PTE(pa) | perm | PTE_V;  //PA2PTE得到页表项的地址位,或上后面的perm得到标志位
  return 0;
}

void destroy_pagetable(pagetable_t pt)
{
  for(int i = 0; i < 512; i++){
    pte_t pte = pt[i];
    if((pte & PTE_V) && (pte & (PTE_R|PTE_W|PTE_X)) == 0){
      uint64 child = PTE2PA(pte);
      destroy_pagetable((pagetable_t)child);
      pt[i] = 0;
    } else if(pte & PTE_V){
        //数据页
    }
  }
  free_page((void*)pt);
}

    // 递归打印页表内容
// pagetable: 当前页表页的指针
// level: 当前层级 (顶层为 2，中间为 1，底层为 0)
void
dump_pagetable(pagetable_t pagetable, int level)
{
  // 遍历当前页表的所有 512 个页表项
  for(int i = 0; i < 512; i++){
    pte_t pte = pagetable[i];

    // 只处理有效位 (PTE_V) 为 1 的项
    if(pte & PTE_V){
      
      // 1. 打印缩进，直观显示层级结构
      // level 2 -> ".. "
      // level 1 -> ".. .. "
      // level 0 -> ".. .. .. "
      for(int j = 0; j < (3 - level); j++){
        printf(".. ");
      }

      // 2. 打印索引、PTE 原值、对应的物理地址
      uint64 pa = PTE2PA(pte);
      printf("%d: pte %p pa %p", i, pte, pa);

      // 3. 打印权限位 (显式标明)
      // V=Valid, R=Read, W=Write, X=Execute, U=User
      printf(" [");
      if(pte & PTE_V) printf("V");
      if(pte & PTE_R) printf("R");
      if(pte & PTE_W) printf("W");
      if(pte & PTE_X) printf("X");
      if(pte & PTE_U) printf("U");
      printf("]");

      printf("\n");

      // 4. 递归处理
      // 判断是否为非叶子节点：
      // RISC-V 规定：如果 V=1 且 R/W/X 全为 0，则指向下一级页表
      if((pte & (PTE_R|PTE_W|PTE_X)) == 0){
        // level - 1 进入下一层
        // pa 是下一级页表的物理地址，在 xv6 内核中可以直接当作指针使用
        dump_pagetable((pagetable_t)pa, level - 1);
      }
    }
  }
}


extern char etext[];

/* 全局内核页表指针 */
pagetable_t kernel_pagetable;

/* 
 * 辅助函数：创建一个空的页表
 * 分配一页内存并将其清零
 */


int map_region(pagetable_t pt, uint64 va, uint64 pa, uint64 size, int perm) {
    if (size % PGSIZE != 0) {
        panic("map_region: size not aligned");
    }

    for (uint64 i = 0; i < size; i += PGSIZE, va += PGSIZE, pa += PGSIZE) {
        if (map_page(pt, va, pa, perm) < 0) {
            return -1;
        }
    }
    return 0;
}

/*
 * 任务 6：实现 kvminit
 * 初始化内核页表并建立直接映射
 */
void kvminit(void) {
    // 1. 创建内核页表
    kernel_pagetable = create_pagetable();

    // 2. 映射内核代码段 (Text Segment)
    // 范围：KERNBASE (0x80000000) 到 etext
    // 权限：只读 (R) + 可执行 (X)
    // 注意：(uint64)etext 获取该符号的地址
    map_region(kernel_pagetable, KERNBASE, KERNBASE, 
               (uint64)etext - KERNBASE, PTE_R | PTE_X);

    // 3. 映射内核数据段 (Data Segment) & 自由内存
    // 范围：从 etext 结束位置直到物理内存顶部 (PHYSTOP)
    // 权限：读 (R) + 写 (W)
    map_region(kernel_pagetable, (uint64)etext, (uint64)etext, 
               PHYSTOP - (uint64)etext, PTE_R | PTE_W);

    // 4. 映射设备（UART0）
    // 为了让内核能打印输出，必须映射 UART 寄存器
    // 权限：读 (R) + 写 (W)
    map_region(kernel_pagetable, UART0, UART0, PGSIZE, PTE_R | PTE_W);
    // 5. 映射触发 page fault 的 MMIO 页（临时最小修复）
map_region(kernel_pagetable, 0x0c201000UL, 0x0c201000UL, PGSIZE, PTE_R | PTE_W);

    
    
}

/*
 * 任务 6：实现 kvminithart
 * 激活内核页表
 */
void kvminithart(void) {
    // 1. 将内核页表的物理根地址写入 satp 寄存器
    // MAKE_SATP 宏负责设置模式(Sv39)和物理页号
    w_satp(MAKE_SATP(kernel_pagetable));

    // 2. 刷新 TLB (Translation Lookaside Buffer)
    // 确保 CPU 使用新的页表映射，而不是缓存的旧映射
    sfence_vma();
}
// Copy from kernel to user.
// Copy len bytes from src to virtual address dstva in a given page table.
// Return 0 on success, -1 on error.
int
copyout(pagetable_t pagetable, uint64 dstva, char *src, uint64 len)
{
  uint64 n, va0, pa0;

  while(len > 0){
    va0 = PGROUNDDOWN(dstva);
    pa0 = walk_create(pagetable, va0);
    if(pa0 == 0)
      return -1;
    n = PGSIZE - (dstva - va0);
    if(n > len)
      n = len;
    memset((void *)(pa0 + (dstva - va0)), src, n);

    len -= n;
    src += n;
    dstva = va0 + PGSIZE;
  }
  return 0;
}

