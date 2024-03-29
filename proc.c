#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;
static int totalTicketCount = 0;
static int g_seed = 0;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

const int PRIORITY_Q[6] = {1, 2, 4, 8, 16, 32};

//should be called locked
static inline int
fastrandSlated(int s) {
  g_seed = ((214013+s)*g_seed+(2531011+s));
  return (g_seed>>16)&0x7FFF;
}

static inline int
fastrand() {
  g_seed = (214013*g_seed+2531011);
  return (g_seed>>16)&0x7FFF;
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->priority = 3;
  p->ticketCount = 10;
  p->pid = nextpid++;
  if(p->pid == 1)
    p->priority = 1;

  p->lastChangeTime = p->creationTime = ticks;
  p->totalWaitTime = 0;
  p->totalCBT = 0;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  int alg, shouldYield;

  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  p->lastChangeTime = ticks;
  totalTicketCount += p->ticketCount;
  release(&ptable.lock);

  pushcli();
  alg = mycpu()->schedAlg;
  shouldYield = alg == 2 || alg == 3;
  popcli();
  if(shouldYield) {
    yield();
  }

}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();
  int alg, shouldYield;

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->first_pid = np->pid;
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;
  //cprintf("[pid = %d]", pid);
  acquire(&ptable.lock);

  np->state = RUNNABLE;
  np->lastChangeTime = ticks;
  totalTicketCount += np->ticketCount;
  release(&ptable.lock);

  pushcli();
  alg = mycpu()->schedAlg;
  shouldYield = alg == 2 || alg == 3;
  popcli();
  if(shouldYield) {
    yield();
  }

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Pass abandoned children to init and check if last thread
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == 0 || p == curproc)
      continue;
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    } else if(p->first_pid == curproc->first_pid || p == curproc->parent) {
      // Other threads or parent might be joined-to/waiting-for this process
      wakeup1(p);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  totalTicketCount -= curproc->ticketCount;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
diagwait(void* arg)
{
  struct procTimes* procTimes = (struct procTimes*)arg;
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->pid != p->first_pid || p->parent != curproc) // if p isn't a thread or the caller process
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        if(procTimes) {
          procTimes->CBT = p->totalCBT;
          procTimes->WT = p->totalWaitTime;
          procTimes->TT = p->lastChangeTime - p->creationTime;
        }
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->first_pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->priority = 0;
        p->creationTime = 0;
        p->lastChangeTime = 0;
        p->totalWaitTime = 0;
        p->totalCBT = 0;
        p->ticketCount = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int
wait(void) {
  return diagwait(0);
}

// Wait for thread with the same first_pid to exit and return its tid.
// Return -1 if this process has no other threads.
int
thread_join(int target_tid)
{
  struct proc *p;
  int targetexists, res;
  struct proc *curproc = myproc();
  if(target_tid == curproc->pid){
    return -2;
  }

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    targetexists = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p == curproc || p->first_pid != curproc->first_pid || p->pid != target_tid)
        continue;
      targetexists = 1;
      if(p->pid == p->first_pid) {
        return -2; // you cant join a process that was creasted with fork
      }

      if(p->state == ZOMBIE){
        // Found one.
        if(p->killed) {
          res = -1;
        } else
          res = 0;
        kfree(p->kstack);
        p->kstack = 0;
        freethread(p->pgdir, PGROUNDDOWN(curproc->tf->esp));
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->first_pid = 0;
        p->priority = 0;
        p->creationTime = 0;
        p->lastChangeTime = 0;
        p->totalWaitTime = 0;
        p->totalCBT = 0;
        p->ticketCount = 0;
        release(&ptable.lock);
        return res;
      }
    }

    // No point waiting if the target doesn't exist.
    if(!targetexists || curproc->killed){
      release(&ptable.lock);
      return -2; // you cant wait on a target that doesn't exist or if your'e dead
    }

    // Wait for target to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p, *minP;
  struct cpu *c = mycpu();
  uint timeQ;
  uint found;
  uint index;
  uint minPP; //minP priority
  int winner;
  c->proc = 0;

  for(int i = 0; i < 1000; i++) {
    fastrandSlated(ticks);
  }

  for(;;){
    // if(!totalTicketCount)
    //   continue;
    // Enable interrupts on this processor.
    sti();
    timeQ = 1;
    found = 0;
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    if(c->RRLastProc < ptable.proc || c->RRLastProc >= &ptable.proc[NPROC])
      c->RRLastProc = ptable.proc;

    //If c->RRLastProc isn't at the beginning of the proc array
    //start form the next index to comply with RR
    if(c->RRLastProc > ptable.proc)
      c->RRLastProc++;

    switch (c->schedAlg) {
    default:
    //cprintf("~default~");
      c->schedAlg = 0;
    case 1:
    //cprintf("~1~");
      timeQ = QUANTUM;
    case 0:
    //cprintf("~0~");
      for(p = c->RRLastProc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE)
          continue;
        c->RRLastProc = p;
        found = 1;
        break;
      }

      if(!found) {
        c->RRLastProc = ptable.proc; // No proc found. Search from the beginning
      }
      break;
    case 2:
      // timeQ = QUANTUM;
    case 3:
    // cprintf("~3~");
      minPP = 0;
      p = c->RRLastProc;
      //Do a full loop on proc array
      //Starting from c->RRLastProc results in using RR algorithm amongst processes with equal priority
      for(index = 0; index < NPROC; index++) {
        if(p->state == RUNNABLE) {
          found = 1;
          if(minPP) {
            if(p->priority < minPP) {
              minPP = p->priority;
              minP = p;
            }
          } else {
            minPP = p->priority;
            minP = p;
          }
          if(minPP == 1)
            break;
        }
        p++;
        if(p >= &ptable.proc[NPROC])
          p = ptable.proc;
      }

      if(found) {
        p = minP;
        c->RRLastProc = minP;
        if(c->schedAlg == 3) {
        timeQ = PRIORITY_Q[p->priority-1];
        }
      } else {
        c->RRLastProc = ptable.proc; // No proc found. Search from the beginning
      }
      break;
    case 4:
      if(totalTicketCount > 0)
        winner = fastrand() % totalTicketCount;
      else
        winner = 0;

      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if(p->state != RUNNABLE)
          continue;
        winner -= p->ticketCount;
        if(winner < 0) {
          found = 1;
          break;
        }
      }
      break;
    }
    if(found) {
      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.

      //cprintf("[pid = %d]", p->pid);
      c->ctr = timeQ;
      c->proc = p;

      p->totalWaitTime += (ticks - p->lastChangeTime);
      p->lastChangeTime = ticks;

      //cprintf("[%d/%d]", fastrand() % totalTicketCount, totalTicketCount);

      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();
  p->totalCBT += ticks - p->lastChangeTime;
  p->lastChangeTime = ticks;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

int
get_policy(void)
{
  return cpus[0].schedAlg;
}

int
change_policy(int policy)
{
  if(policy < 0 || policy > 4)
    return -1;
  int i;
  pushcli();
  for (i = 0; i < ncpu; ++i) {
    cpus[i].schedAlg = policy;
  }
  popcli();
  //TODO: Check if needed
  // for (i = 0; i<ncpu; ++i){
  //   if(cpus[i].schedAlg != policy){
  //     return -1;
  //   }
  // }
  return policy;
}

void
set_priority(int prio)
{
  if(prio < 1 || prio > 6)
  prio = 5;
  myproc()->priority = prio;
}

void
get_proc_times(void* arg)
{
  struct proc *p = myproc();
  struct procTimes* procTimes = (struct procTimes*)arg;
  p->totalCBT += ticks - p->lastChangeTime;
  p->lastChangeTime = ticks;
  procTimes->TT = ticks - p->creationTime;
  procTimes->WT = p->totalWaitTime;
  procTimes->CBT = p->totalCBT;
}

int
get_ticketCount(void) {
  return myproc()->ticketCount;
}

void
set_ticketCount(int ticketCount) {
  struct proc* p;
  if(ticketCount > 0) {
    p = myproc();
    totalTicketCount += ticketCount - p->ticketCount;
    p->ticketCount = ticketCount;
  }
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;
  totalTicketCount -= p->ticketCount;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
      p->lastChangeTime = ticks;
      totalTicketCount += p->ticketCount;
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  int alg, shouldYield;
  struct proc *p;
  int changedToRunnable = 0;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING) {
        p->state = RUNNABLE;
        p->lastChangeTime = ticks;
        totalTicketCount += p->ticketCount;
        changedToRunnable = 1;
      }
      release(&ptable.lock);

      if(changedToRunnable) {
        pushcli();
        alg = mycpu()->schedAlg;
        shouldYield = alg == 2 || alg == 3;
        popcli();
        if(shouldYield)
          yield();
      }

      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}


// Should only be used in combination with detachpage
uint
getpage(void)
{
  struct proc *curproc = myproc();
  return palloc(curproc->pgdir, PGROUNDUP(curproc->sz));
}

uint
detachpage(uint loc)
{
  return unmappages(myproc()->pgdir, (void*)loc, PGSIZE);
}

int
thread_create(void* stack)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();
  uint sp = (uint)stack;
  uint spage = PGROUNDDOWN(sp);
  uint orig_stack_beg = PGROUNDDOWN(curproc->tf->esp);
  int alg, shouldYield;

  // Allocate process.
  if((np = allocproc()) == 0){
    cprintf("allocproc failed\n");
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = linkuvm(curproc->pgdir, curproc->sz, orig_stack_beg, sp)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    cprintf("linkuvm failed\n");
    return -1;
  }
  np->first_pid = curproc->first_pid;
  np->sz = orig_stack_beg+PGSIZE;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  // Clear %eax so that thread_create returns 0 in the new thread.
  np->tf->eax = 0;
  np->tf->esp = orig_stack_beg + (sp - spage);
  np->tf->eip = *((uint*)(np->tf->esp));  // function to start executing
  np->tf->esp += sizeof(uint*); // set esp to return location (ttrap)

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  np->lastChangeTime = ticks;
  totalTicketCount += np->ticketCount;
  release(&ptable.lock);

  pushcli();
  alg = mycpu()->schedAlg;
  shouldYield = alg == 2 || alg == 3;
  popcli();
  if(shouldYield) {
    yield();
  }

  return pid;
}
