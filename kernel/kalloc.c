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

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
}kmem[NCPU];

void
kinit()
{
  for(int i = 0;i<NCPU;i++){
    initlock(&kmem[i].lock, "kmem");
  }
    freerange(end, (void*)PHYSTOP);
 // }
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

  push_off();//关闭中断
  int mycpu = cpuid();
  pop_off();//恢复中断

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem[mycpu].lock);
  r->next = kmem[mycpu].freelist;
  kmem[mycpu].freelist = r;
  release(&kmem[mycpu].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  int mycpu = cpuid();
  pop_off();

  acquire(&kmem[mycpu].lock);
  r = kmem[mycpu].freelist;
  if(r){
    kmem[mycpu].freelist = r->next;
    release(&kmem[mycpu].lock);
  }else{//从其它cpu的freelist上偷页
    int anothercpu;
    //release(&kmem[mycpu].lock);
    for(int j = 0;j<NCPU;j++){
      anothercpu = (mycpu + j) % NCPU;
      //acquire(&kmem[anothercpu].lock);
      if(kmem[anothercpu].freelist)//找到非空freelist就退出循环
	break;
      //release(&kmem[anothercpu].lock);
    }
    release(&kmem[mycpu].lock);
    acquire(&kmem[anothercpu].lock);
    r = kmem[anothercpu].freelist;
    if(r)
      kmem[anothercpu].freelist = r->next;
    release(&kmem[anothercpu].lock);
  }
  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
