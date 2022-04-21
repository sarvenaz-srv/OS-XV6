#include "types.h"
#include "stat.h"
#include "user.h"

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
  char *sp, *stack = kalloc();
  int pid;
  sp = stack + PGSIZE;

  if(arg) {
    sp = (sp - (strlen(arg) + 1)) & ~3;
    memmove(sp, arg, strlen(arg));
    ustack[3] = sp;
    ustack[4] = 0;
    argc = 1;

  } else {
    ustack[3] = 0;
    argc = 0;
  }

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  memmove(sp, ustack, (3+argc+1)*4);

  pid = thread_create(stack);
  if(pid) {
    fn(arg);
    exit();
  }
  return 0;
}
