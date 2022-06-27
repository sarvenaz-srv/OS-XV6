#include "types.h"
#include "stat.h"
#include "user.h"


int main(void) {
  for(int i = 0; i < 10; i++) {
    printf(1, "i = %d\n", i);
    if(!fork()) {
      int pid = getpid();
      for(int j = 0; j < 100; j++)
        printf(1, "/%d/ : /%d/\n", pid, j);
      struct procTimes procTimes = get_proc_times();
      printf(1, "CPU Burst Time: %d\nTurnaround Time: %d\nWait Time: %d", procTimes.CBT, procTimes.TT, procTimes.WT);
      exit();
    }
  }

  for(int i = 0; i < 10; i++) {
    printf(1, "waited: %d\n", wait());
  }
  exit();
}
