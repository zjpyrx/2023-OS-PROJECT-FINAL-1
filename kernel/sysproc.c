#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "kernel/sysinfo.h"

uint64
sys_trace(void)
{
  int mask;
  argint(0, &mask);  //argint函数获取系统调用函数的整型参数
  if (mask < 0)  //检查mask的值是否小于0,小于0表示传入的掩码值无效
    return -1;
  myproc()->mask = mask;  //myproc()函数用于获取当前进程的指针。将mask值赋给当前进程的mask成员变量
  return 0;
}

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

//新增sysinfo
uint64
sys_sysinfo(void)
{
  struct sysinfo info;    // addr代表的是userspace的内存地址，info处于kernel中，需要复制到user中
  uint64 addr;
  if (argaddr(0, &addr) < 0)  //argaddr函数用于获取地址,此处将索引为0的参数的地址保存在addr变量中。
    return -1;
  info.nproc = proc_getnum();  //获取当前进程数，将结果保存在info.nproc中。
  info.freemem = getfreememSize();  //获取系统的空闲可用内存大小
  if (copyout(myproc()->pagetable, addr, (char*)&info, sizeof(info)) < 0) {  //copyout，将info结构体从kernel复制到user空间的地址addr处。
    return -1;
  }
  else {
    return 0;
  }
  return 0;
}
