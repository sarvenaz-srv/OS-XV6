#include "types.h"
#include "stat.h"
#include "user.h"
#include "mmu.h"

int
thread_creator(void (*fn) (void*), void *arg)
{

  /*
    Order of Operations:
      1-Fix input as needed
      2-Create new Thread/Process
      3-Set Function
  */
  uint argc, ustack[3+2];
  uint sp, stack = getpage();
  int res, i;
  char* arg0;
  sp = stack + PGSIZE;
  printf(1, "sthread1: stack is %p & sp is %p\n", stack, sp);
  if(arg) {
    sp = (sp - (strlen(arg) + 1)) & ~3;
    printf(1, "sthread2: strlen(arg) = %p & arg = %s & sp = %p\n", strlen(arg), arg, sp);
    memmove((void*)sp, (void*)arg, strlen(arg)+1);
    ustack[3] = sp;
    printf(1, "sthread3: sp is %p & arg is %s & strlen(arg) = %d\n", sp, (char*)(ustack[3]), strlen((char*)(ustack[3])));
    ustack[4] = 0;
    argc = 1;

  } else {
    ustack[3] = 0;
    argc = 0;
  }

  ustack[0] = (uint)fn;  // This value is used to trasfer fn to kernel level
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer
  printf(1, "sthread4: sp is %p & fn is %p\n", sp, (uint)fn);

  sp -= (3+argc+1) * 4;
  memmove((void*)sp, (void*)ustack, (3+argc+1)*4);
  arg0 = (char*)(((uint*)(((uint*)sp)[2]))[0]);
  printf(1, "sthread5: sp is %p & arg is %s & strlen(arg) = %d & fn is %p\n", sp, arg0, strlen(arg0), *(uint*)sp);

  // Stack dump
  printf(1, "*\n*char*\n*\n");
  for(i = sp; i < stack+PGSIZE; i++)
    printf(1, "%c", *((char*)i));
  printf(1, "\n*\nvoid*\n*\n");
  for(i = sp; i < stack+PGSIZE; i+=sizeof(void*))
    printf(1, "%p, ", ((void*)i));
  printf(1, "\n*\nuint\n*\n");
  for(i = sp; i < stack+PGSIZE; i+=sizeof(uint))
    printf(1, "%p, ", ((uint)i));
  printf(1, "\n*\n*uint*\n*\n");
  for(i = sp; i < stack+PGSIZE; i+=sizeof(uint*))
    printf(1, "%p, ", (*(uint*)i));
  printf(1, "\n*\n*\n*\n");

  sp = detachpage(stack) + (sp - stack);
  printf(1, "sthread6: sp is %p & fn is %p\n", sp, sp);

  // Stack dump
  printf(1, "\n*\nvoid*\n*\n");
  for(i = sp; i < PGROUNDUP(sp); i+=sizeof(void*))
    printf(1, "%p, ", ((void*)i));
  printf(1, "\n*\nuint\n*\n");
  for(i = sp; i < PGROUNDUP(sp); i+=sizeof(uint))
    printf(1, "%p, ", ((uint)i));
  printf(1, "\n*\n*\n*\n");

  res = thread_create((void*)sp);
  printf(1, "sthread7: res is %d\n", res);
  return res;
}
