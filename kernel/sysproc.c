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
  uint64 vaddr;
  int num;
  uint64 res_addr;
  argaddr(0, &vaddr); //获取第一个参数：虚拟地址
  argint(1, &num); //获取第二个参数：页数
  argaddr(2, &res_addr); //获取第三个参数：结果存储地址

  struct proc* p = myproc();
  pagetable_t pagetable = p->pagetable;
  uint64 res = 0;

  for (int i = 0; i < num; i++) { //遍历指定页数
    pte_t* pte = walk(pagetable, vaddr + PGSIZE * i, 1); //连续获取PTE
    if (*pte & PTE_A) { //如果页面已经被访问过
      *pte &= (~PTE_A); //清除访问标志位
      res |= (1L << i); //将相应的位掩码位置为 1
    }
  }

  copyout(pagetable, res_addr, (char*)&res, sizeof(uint64)); //复制到用户空间
  return 0;
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
