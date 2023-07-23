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
  argint(0, &mask);  //argint������ȡϵͳ���ú��������Ͳ���
  if (mask < 0)  //���mask��ֵ�Ƿ�С��0,С��0��ʾ���������ֵ��Ч
    return -1;
  myproc()->mask = mask;  //myproc()�������ڻ�ȡ��ǰ���̵�ָ�롣��maskֵ������ǰ���̵�mask��Ա����
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

//����sysinfo
uint64
sys_sysinfo(void)
{
  struct sysinfo info;    // addr�������userspace���ڴ��ַ��info����kernel�У���Ҫ���Ƶ�user��
  uint64 addr;
  if (argaddr(0, &addr) < 0)  //argaddr�������ڻ�ȡ��ַ,�˴�������Ϊ0�Ĳ����ĵ�ַ������addr�����С�
    return -1;
  info.nproc = proc_getnum();  //��ȡ��ǰ�������������������info.nproc�С�
  info.freemem = getfreememSize();  //��ȡϵͳ�Ŀ��п����ڴ��С
  if (copyout(myproc()->pagetable, addr, (char*)&info, sizeof(info)) < 0) {  //copyout����info�ṹ���kernel���Ƶ�user�ռ�ĵ�ַaddr����
    return -1;
  }
  else {
    return 0;
  }
  return 0;
}
