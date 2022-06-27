#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
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

int
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

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


// Should only be used in combination with sys_detachpage
int
sys_getpage(void)
{
  return getpage();
}

int
sys_detachpage(void)
{
  uint loc;
  if(argint(0, (int*)&loc) < 0)
    return -1;
  return detachpage(loc);
}

int
sys_thread_create(void) // void* stack
{
  uint stack;
  if(argint(0, (int*)&stack) < 0)
    return -1;
  return thread_create((void*)stack);
}

int
sys_thread_id(void)
{
  return myproc()->pid;
}

int
sys_thread_join(void) //int id
{
  int id;
  if(argint(0, &id) < 0)
    return -1;
  return thread_join(id);
}

int
sys_change_policy(void)
{
  int pol;
  if(argint(0, &pol) <0)
    return -1;
  return change_policy(pol);
}

int
sys_get_priority(void)
{
  return get_priority();
}

int
sys_set_priority(void)
{
  int prio;
  if(argint(0, &prio)<0){
    return -1;
  }
  set_priority(prio);
  return 1;
}

void
sys_get_proc_times(void)
{
  char* arg;
  argptr(0, &arg, sizeof(struct procTimes));
  get_proc_times(arg);
}
