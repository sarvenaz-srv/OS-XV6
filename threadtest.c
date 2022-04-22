#include "types.h"
#include "stat.h"
#include "user.h"

void testf2(char x) {
  int i;
  for(i = 0; i < 100; i++) {
    printf(1, "%c", x);
  }
  printf(1, "\n");
}

void testf(void* x)
{
  int i;
  printf(1, "testf, testing thread_id: tid = %d\n", thread_id());
  for(i = 0; i < 100; i++) {
    printf(1, "%s", (char*)x);
  }
  printf(1, "\n");
  testf2('2');
  printf(1, "system uptime is %d\n", uptime());
  exit();
}

int main(void)
{
  int i, tid;
  char* dot = "1";
  tid = thread_creator(testf, dot);
  if (tid > 0) {
    printf(1, "THREAD CREATED\n");
    if(thread_join(tid) == tid)
      printf(1, "thread_join successful\n");
    else
      printf(1, "thread_join failed\n");
    for(i = 0; i < 100; i++)
      printf(1, "3");
    printf(1, "\nthread_create successful, tid = %d\n", tid);
  } else
    printf(1, "thread_create failed\n");

  exit();
}
