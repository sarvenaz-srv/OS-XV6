#include "types.h"
#include "stat.h"
#include "user.h"

int Base, Limit;

void increase(void* arg);

int main(void)
{
  int res, ntid;
  Base = 0;
  Limit = 3;
  printf(1, "Base = %d, Limit = %d\n", Base, Limit);
  ntid = thread_creator(increase, 0);
  res = thread_join(ntid);
  if(res == 0) {
    printf(1, "[ID] %d => [Success] 0\n", getpid());
  } else if(res == -1) {
    printf(1, "[ID] %d => [Failed] -1\n", getpid());
  } else {
    printf(1, "main, thread not found");
  }
  exit();
}

void increase(void* arg) {
  int res, tid, ntid;
  Base++;
  if(Base < Limit) {
    tid = thread_id();
    ntid = thread_creator(increase, 0);
    res = thread_join(ntid);
    if(res == 0) {
      printf(1, "[ID] %d => [Success] 0\n", tid);
    } else if(res == -1) {
      printf(1, "[ID] %d => [Failed] -1\n", tid);
    } else {
      printf(1, "increase, thread not found");
    }
  } else {
    exit();
  }
}
