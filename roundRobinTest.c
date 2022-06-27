#include "types.h"
#include "stat.h"
#include "user.h"


int main(void) {
  int origPri = get_priority();
  change_policy(1);
  for(int i = 0; i < 10; i++) {
    printf(1, "i = %d\n", i);
    if(!fork()) {
      int pid = getpid();
      for(int j = 0; j < 1000; j++)
        printf(1, "/%d/ : /%d/\n", pid, j);
      struct procTimes procTimes;
      get_proc_times(&procTimes);
      printf(1, "CPU Burst Time: %d\nTurnaround Time: %d\nWait Time: %d\n", procTimes.CBT, procTimes.TT, procTimes.WT);
      exit();
    }
  }

  for(int i = 0; i < 10; i++) {
    if(wait() < 0)
      printf(1, "No Children Left!!!\n");
  }
  set_priority(origPri);
  exit();
}
