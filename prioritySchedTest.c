#include "types.h"
#include "stat.h"
#include "user.h"

#define MAX_DELTA 90

int main(void) {
  int origPolicy = get_policy();
  struct procTimes procTimes[30] = {{0, 0, 0}};
  struct procTimes tmpProcTimes = {0, 0, 0};
  int pids[MAX_DELTA];
  int pid, firstPid = getpid();
  change_policy(2);
  set_priority(1);
  for(int i = 0; i < 30; i++) {
    pid = fork();
    if(pid) {
      pids[pid - firstPid] = i;
    } else {
      pid = getpid();
      set_priority((34 - i)/5);
      for(int j = 0; j < 250; j++)
        printf(1, "/%d/ : /%d/\n", pid, j);
      exit();
    }
  }

  for(int i = 0; i < 30; i++) {
    pid = diagwait(&tmpProcTimes);
    if(pid < 0)
      printf(1, "No Children Left!!!\n");
    else {
      procTimes[pids[pid - firstPid]] = tmpProcTimes;
    }
  }
  printf(1, "~Done Waiting~\n");
  
  float CBTavg = 0, WTavg = 0, TTavg = 0;
  for(int i = 0; i < 30; i++) {
    printf(1, "CPU Burst Time: %d\tTurnaround Time: %d\tWait Time: %d\n", procTimes[i].CBT, procTimes[i].TT, procTimes[i].WT);
    CBTavg += (float)procTimes[i].CBT / 30;
    WTavg += (float)procTimes[i].WT / 30;
    TTavg += (float)procTimes[i].TT / 30;
  }
  printf(1, "\nAVG CPU Burst Time: %d\nAVG Turnaround Time: %d\nAVG Wait Time: %d\n", (int)CBTavg, (int)TTavg, (int)WTavg);

  change_policy(origPolicy);
  exit();
}
