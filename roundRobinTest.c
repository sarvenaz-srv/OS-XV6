#include "types.h"
#include "stat.h"
#include "user.h"


int main(void) {
  int origPolicy = get_policy();
  struct procTimes procTimes[10] = {{0, 0, 0}};
  change_policy(1);
  for(int i = 0; i < 10; i++) {
    //printf(1, "i = %d\n", i);
    if(!fork()) {
      int pid = getpid();
      for(int j = 0; j < 1000; j++)
        printf(1, "/%d/ : /%d/\n", pid, j);
      // struct procTimes procTimes;
      // get_proc_times(&procTimes);
      // printf(1, "CPU Burst Time: %d\tTurnaround Time: %d\tWait Time: %d\n", procTimes.CBT, procTimes.TT, procTimes.WT);

      exit();
    }
    // diagwait(&(procTimes[i]));
  }


  for(int i = 0; i < 10; i++) {
    if(diagwait(&(procTimes[i])) < 0)
      printf(1, "No Children Left!!!\n");
  }
  printf(1, "~Done Waiting~\n");

  float CBTavg = 0, WTavg = 0, TTavg = 0;
  for(int i = 0; i < 10; i++) {
    printf(1, "CPU Burst Time: %d\tTurnaround Time: %d\tWait Time: %d\n", procTimes[i].CBT, procTimes[i].TT, procTimes[i].WT);
    CBTavg += (float)procTimes[i].CBT / 10;
    WTavg += (float)procTimes[i].WT / 10;
    TTavg += (float)procTimes[i].TT / 10;
  }
  printf(1, "\nAVG CPU Burst Time: %d\nAVG Turnaround Time: %d\nAVG Wait Time: %d\n", (int)CBTavg, (int)TTavg, (int)WTavg);
  change_policy(origPolicy);
  exit();
}
