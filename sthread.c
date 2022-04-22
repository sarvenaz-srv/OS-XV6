#include "types.h"
#include "stat.h"
#include "user.h"
#include "mmu.h"

void
myexit(void)
{
  exit();
}

int
thread_creator(void (*fn) (void*), void *arg)
{
  uint argc, ustack[3];
  uint sp, stack = getpage();
  sp = stack + PGSIZE;
  if(arg) {
    sp = (sp - (strlen(arg) + 1)) & ~3;
    memmove((void*)sp, (void*)arg, strlen(arg)+1);
    ustack[1] = sp;
    ustack[2] = 0;
    argc = 1;

  } else {
    ustack[1] = 0;
    argc = 0;
  }

  ustack[0] = (uint)fn;  // This value is used to trasfer fn to kernel level
  sp -= (2+argc) * 4;
  memmove((void*)sp, (void*)ustack, (2+argc)*4);
  sp = detachpage(stack) + (sp - stack);
  return thread_create((void*)sp);
}
