// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.


//lab5新增
//KERNBASE == 0x80000000
static int reference_count[(PHYSTOP - KERNBASE) / PGSIZE]; //表示系统中物理页面的总数

static int idx_rc(uint64 pa) { //获取对应物理页面在reference_count数组中的下标值
  return (pa - KERNBASE) / PGSIZE;
}
void add_rc(uint64 pa) { // 增加物理页面的引用计数值
  reference_count[idx_rc(pa)]++;
}
void sub_rc(uint64 pa) { // 减少物理页面的引用计数值
  reference_count[idx_rc(pa)]--;
}



struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");
  //lab5新增。引用数大于1:该页面被多个进程或内核模块共享,只需要将引用计数值减1，不需要立即释放
  if (reference_count[idx_rc((uint64)pa)] > 1) {
    sub_rc((uint64)pa);
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);

  reference_count[idx_rc((uint64)pa)] = 0;//新增
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  //lab5新增，在分配页的时候初始化引用计数（设置为1）
  if (r)
    reference_count[idx_rc((uint64)r)] = 1;

  return (void*)r;
}
