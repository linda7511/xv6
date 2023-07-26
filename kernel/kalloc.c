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
extern int refnum[];
struct spinlock reflock;

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;

 // struct spinlock reflock;
  //char *paref;//refnum数组起始位置
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&reflock,"refnum");
 // kmem.paref = refnum;
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
  
  acquire(&reflock);
  if(--refnum[((uint64)pa-KERNBASE)/PGSIZE]<=0){
  //先减少引用计数，如果小于等于0就真的释放

    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run*)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
  }
  release(&reflock);
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
  //printf("kalloc lock1\n");
 // acquire(&reflock);
  //printf("kalloc lock2\n");
  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
    acquire(&reflock);
    refnum[((uint64)r-KERNBASE)/PGSIZE] = 1;//分配时只有一个进程关联到该页，所以设置为1
    release(&reflock);
  }
 // release(&reflock);
  return (void*)r;
}
