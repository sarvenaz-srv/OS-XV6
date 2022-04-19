#include "types.h"
#include "stat.h"
#include "user.h"

int main(void)
{
  printf(1, "thredtest:\n");

  int stack = 23;
  if (thread_create((void*)&stack) == 23)
    printf(1, "thread_id successful\n");
  else
    printf(1, "thread_id failed\n");

  if (thread_id() == 37)
    printf(1, "thread_id successful\n");
  else
    printf(1, "thread_id failed\n");

  if(thread_join(56) == 56)
    printf(1, "thread_join successful\n");
  else
    printf(1, "thread_join failed\n");

  exit();
}
