#include "types.h"
#include "stat.h"
#include "user.h"

int Base, Limit;

void increase(void* arg);
void test(void* arg);

int main(void)
{
  int res, ntid;
  Base = 0;
  Limit = 3;
  printf(1, "Base = %d, Limit = %d\n", Base, Limit);
  ntid = thread_creator(increase, 0);
  if(ntid <= 0)
    printf(1, "threadtest failed\n");
  else {
    res = thread_join(ntid);
    if(res)
      printf(1, "threadtest failed\n");
    else
      printf(1, "threadtest successful\n");
  }
  exit();
}

void increase(void* arg) {
  int res, tid = thread_id(), ntid;
  int preVal = ++Base;
  if(Base < Limit) {
    ntid = thread_creator(increase, 0);
    if(ntid <= 0)
      res = -1;
    else
      res = thread_join(ntid);
    if(res == 0) {
      printf(1, "[%d] %d => [Success]\n", tid, preVal);
    } else if(res == -1) {
      printf(1, "[%d] %d => [Failed]\n", tid, preVal);
    } else {
      printf(1, "[%d] %d => [Join not valid]\n", tid, preVal);
    }
  } else
    printf(1, "[%d] %d => [New thread not needed]\n", tid, preVal);
}
