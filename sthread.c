#include "types.h"
#include "stat.h"
#include "user.h"
#include "mmu.h"

void
ttrap(void)
{
  int tid = thread_id();
  if(tid)
    exit();
  else
    printf(1, "Calling ttrap on a non-successful thread\n");
}

int
thread_creator(void (*fn) (void*), void *arg)
{
  uint argc, ustack[4];
  uint sp, stack = getpage();
  sp = stack + PGSIZE;
  if(arg) {
    sp = (sp - (strlen(arg) + 1)) & ~3;
    memmove((void*)sp, (void*)arg, strlen(arg)+1);
    ustack[2] = sp;
    ustack[3] = 0;
    argc = 1;

  } else {
    ustack[2] = 0;
    argc = 0;
  }

  ustack[0] = (uint)fn;  // This value is used to trasfer fn to kernel level
  ustack[1] = (uint)ttrap;
  sp -= (argc+3) * 4;
  memmove((void*)sp, (void*)ustack, (argc+3)*4);
  sp = detachpage(stack) + (sp - stack);
  return thread_create((void*)sp);
}
