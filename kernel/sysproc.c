#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 va;//存放第一个用户页面的起始虚拟地址
  uint64 mask;//存放位掩码的用户空间缓冲区
  int len;//页数

  struct proc *p = myproc();//获取当前进程
  
  pagetable_t pagetable = 0;//用于遍历页表
  unsigned int procmask = 0;//虚拟地址区域访问性掩码
  pte_t *pte;//指向页表

  //获取用户空间传递的参数
  if(argaddr(0,&va)<0||argint(1,&len)<0||argaddr(2,&mask)<0)
    return -1;
  //设置可扫描页数的上限为64，因为mask类型uint64最多存储64位的值
  if(len>sizeof(int)*8)
    len = sizeof(int)*8;

  for(int i = 0; i < len; i++){
    pagetable = p->pagetable;//初始化pagetable为当前进程的页表
    if(va >= MAXVA)
      panic("pgaccess");
    for(int level =2; level >0; level--){//遍历到0级页表项
      pte = &pagetable[PX(level,va)];
      if(*pte & PTE_V){//页表项有效
        pagetable = (pagetable_t)PTE2PA(*pte);//进入下一级页表
      }else{
        return -1;//页表项无效表示虚拟地址不可访问
      }
    }
    pte = &pagetable[PX(0,va)];//获取0级页表项

    if(pte == 0||(*pte&PTE_V)==0||(*pte&PTE_U)==0)//虚拟地址不可访问
      return -1;
    if(*pte & PTE_A){//页面已经被访问过
      procmask = procmask | (1L << i);//掩码对应的位设置为1
      *pte = *pte & (~PTE_A);//将页表项pte的PTE_A位清零
    }
    va += PGSIZE;//下一个虚拟地址
  }
  pagetable = p->pagetable;//恢复指向当前进程的2级页表
  return copyout(pagetable,mask,(char *)&procmask,sizeof(unsigned int));//将掩码复制到用户空间的mask缓冲区
}
#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
