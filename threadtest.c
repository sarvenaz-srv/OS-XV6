#include "sthread.c"

void testf(void* x)
{
  int i;
  printf(1, "HI THIS IS TEST FUNC ID = %d\n", thread_id());
  for(i = 0; i < 1000; i++) {
    printf(1, ".");
    //printf(1, "%s", (char*)x);
  }
  exit();
}

int main(void)
{
  int i, tid;
  uint stackadr;
  char* dot = ".";
  tid = thread_creator(testf, dot);
  if (tid > 0)
    printf(1, "thread_create successful, tid = %d\n", tid);
  else {
    printf(1, "thread_create failed\n");
    exit();
  }
  //thread_join(tid);
  if(thread_join(tid) == tid)
    printf(1, "thread_join successful\n");
  else
    printf(1, "thread_join failed\n");
  for(i = 0; i < 1000; i++)
    printf(1, "!");

  exit();
}
