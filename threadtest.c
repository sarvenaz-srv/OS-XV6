#include "types.h"
#include "stat.h"
#include "user.h"

void testf(void* x)
{
  printf(1, "HI this is testf\n");
}

int main(void)
{
  int i, j;
  uint stackadr;
  for(i = 0; i < 1000; i++) {
    printf(1, "thredtest %d:\n", i);

    int stack = 23;
    //thread_creator(testf, 0);
    if (stack == 23)
      printf(1, "thread_id successful\n");
    else
      printf(1, "thread_id failed\n");

    int tid = thread_id();
    if (tid)
      printf(1, "thread_id successful, tid = %d\n", tid);
    else
      printf(1, "thread_id failed\n");

    if(thread_join(56) == 56)
      printf(1, "thread_join successful\n");
    else
      printf(1, "thread_join failed\n");
  }
  exit();
}
