// Sleeping locks

#include "types.h"
#include "defs.h"
#include "param.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "mutex.h"

void initsleeplock(struct sleeplock *lk, char *name)
{
  initlock(&lk->lk, "sleep lock");
  lk->name = name;
  lk->locked = 0;
  lk->pid = 0;
}

void acquiresleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  while (lk->locked)
  {
    sleep(lk, &lk->lk);
  }
  lk->locked = 1;
  lk->pid = myproc()->pid;
  release(&lk->lk);
}

void releasesleep(struct sleeplock *lk)
{
  acquire(&lk->lk);
  lk->locked = 0;
  lk->pid = 0;
  wakeup(lk);
  release(&lk->lk);
}

int holdingsleep(struct sleeplock *lk)
{
  int r;

  acquire(&lk->lk);
  r = lk->locked && (lk->pid == myproc()->pid);
  release(&lk->lk);
  return r;
}

void sys_macquire(void)
{

  mutex *m;
  if (argptr(0, (void *)&m, sizeof(struct mutex)) < 0)
    return; // failure
  macquire(m);
}

void sys_mrelease()
{
  mutex *m;
  if (argptr(0, (void *)&m, sizeof(struct mutex)) < 0)
    return; // failure

  mrelease(m);
}

void sys_minit()
{
  mutex *m;
  if (argptr(0, (void *)&m, sizeof(struct mutex)) < 0)
    return; // failure

  minit(m);
}

void minit(mutex *m)
{
  initlock(&m->lk, "sleep lock");

  m->locked = 0;
  m->pid = 0;
}

void mrelease(mutex *m)
{
  acquire(&m->lk);
  m->locked = 0;
  m->pid = 0;
  m->nice = 21;

  // Set the lockNice value of each waiting thread to the nice value:
  for (int i = 0; i < m->num_waiters; i++) {
    m->waiters[i]->lockNice = m->nice;
  }
  m->num_waiters = 0; // Reset the number of waiters

  wakeup(m);
  release(&m->lk);
}

void macquire(mutex *m)
{
  acquire(&m->lk);
  while (m->locked) // if the lock is already locked, sleep
  {
    // Register this thread as a waiter:
    m->waiters[m->num_waiters++] = myproc();

    sleep(m, &m->lk);
  }
  m->locked = 1;            // lock acquired
  m->nice = myproc()->nice; // set lock's nice value to the process's nice value
  m->pid = myproc()->pid;   // set lid's pid to the process's pid
  release(&m->lk);          // release lock's lock
}

int sys_nice(void)
{
  int inc;
  if (argint(0, &inc) < 0)
    return -1;
  return nice(inc);
}

int nice(int inc) // increment the nice value of the process
{
  struct proc *p = myproc();
  int new = p->nice + inc;

  if (new < -20) // cap new niceness in range [-20, 19]
    new = -20;
  if (new > 19)
    new = 19;

  p->nice = new;
  if (p->niceChanged == 0) // check to see if the lock nice has been changed, if not keep it line with the process nice
  {
    p->lockNice = new;
  }
  return 0;
}