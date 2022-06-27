#include "types.h"
#include "stat.h"
#include "user.h"


int main(void) {
  int origPolicy = get_policy();
  change_policy(2);
  set_priority(1);
  for(int i = 0; i < 30; i++) {
    printf(1, "i = %d\n", i);
    if(!fork()) {
      int pid = getpid();
      set_priority((34 - i)/5);
      for(int j = 0; j < 250; j++)
        printf(1, "/%d/ : /%d/\n", pid, j);
      struct procTimes procTimes;
      get_proc_times(&procTimes);
      printf(1, "CPU Burst Time: %d\nTurnaround Time: %d\nWait Time: %d\n", procTimes.CBT, procTimes.TT, procTimes.WT);
      exit();
    }
  }

  for(int i = 0; i < 30; i++) {
    if(wait() < 0)
      printf(1, "No Children Left!!!\n");
  }
  change_policy(origPolicy);
  exit();
}
