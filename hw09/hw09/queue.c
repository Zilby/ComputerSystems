#include <stdlib.h>
#include <assert.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <stdio.h>

#include "queue.h"

// TODO: Make this an interprocess queue.

queue*
make_queue()
{
  int pages = 1 + sizeof(queue) / 4096;
  queue* qq = mmap(0, pages * 4096, PROT_READ | PROT_WRITE,
		   MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  qq->qii = 0;
  qq->qjj = 0;
  
  sem_init(&(qq->isem), 1, QUEUE_SIZE);
  sem_init(&(qq->osem), 1, 0);
  return qq;
}

void
free_queue(queue* qq)
{
  assert(qq->qii == qq->qjj);
  int pages = 1 + sizeof(queue) / 4096;
  int rv = munmap(qq, pages);
  assert(rv == 0);
}

void
queue_put(queue* qq, job msg)
{
  int rv;
  rv = sem_wait(&(qq->isem));
  assert(rv == 0);
  unsigned int ii = atomic_fetch_add(&(qq->qii), 1);
  qq->jobs[ii % QUEUE_SIZE] = msg;

  rv = sem_post(&(qq->osem));
  assert(rv == 0);
}

job
queue_get(queue* qq)
{
  int rv;
  rv = sem_wait(&(qq->osem));
  assert(rv == 0);
  unsigned int jj = atomic_fetch_add(&(qq->qjj), 1);
  job temp = qq->jobs[jj % QUEUE_SIZE];
  
  rv = sem_post(&(qq->isem));
  assert(rv == 0);
  
  return temp;
}

