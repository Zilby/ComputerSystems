#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "queue.h"
#include "factor.h"

pthread_mutex_t mutex;
pthread_cond_t  condv;
pthread_t* threads;
int threadcount;
int completed;
int jobcount;
int resultcount;

// FIXME: Shared mutable data
static queue* iqueue;
static queue* oqueue;

void 
factor_init(int i, int j)
{
  pthread_mutex_init(&mutex, 0);
  pthread_cond_init(&condv, 0);
  if (iqueue == 0) iqueue = make_queue();
  if (oqueue == 0) oqueue = make_queue();
  completed = 0;
  threadcount = i;
  jobcount = j;
  resultcount = j;
  threads = malloc(sizeof(pthread_t) * (i + 1));
  for(int ii = 0; ii < i; ++ii) {
    int rv = pthread_create(&(threads[ii]), 0, &run_jobs, 0);
    assert(rv == 0); 
  }
}

void
factor_cleanup()
{
  for (int ii = 0; ii < threadcount; ++ii)
    {
      pthread_join (threads[ii], NULL);
    } 
  free_queue(iqueue);
  iqueue = 0;
  free_queue(oqueue);
  oqueue = 0;
}

factor_job* 
make_job(int128_t nn)
{
  factor_job* job = malloc(sizeof(factor_job));
  job->number  = nn;
  job->factors = 0;
  return job;
}

void 
free_job(factor_job* job)
{
  if (job->factors) {
    free_ivec(job->factors);
  }
  free(job);
}

void 
submit_job(factor_job* job)
{
  queue_put(iqueue, job);
}

factor_job* 
get_result()
{
  if(resultcount == 0) {
    close_output();
  }
  resultcount--;
  return queue_get(oqueue);
}

static
int128_t
isqrt(int128_t xx)
{
  double yy = ceil(sqrt((double)xx));
  return (int128_t) yy;
}

ivec*
factor(int128_t xx)
{
  ivec* ys = make_ivec();

  while (xx % 2 == 0) {
    ivec_push(ys, 2);
    xx /= 2;
  }

  for (int128_t ii = 3; ii <= isqrt(xx); ii += 2) {
    int128_t x1 = xx / ii;
    if (x1 * ii == xx) {
      ivec_push(ys, ii);
      xx = x1;
      ii = 1;
    }
  }

  ivec_push(ys, xx);
  return ys;
}

void*
run_jobs(void* arg)
{
  // FIXME: This should happen in its own thread.

  // FIXME: We should block on an empty queue waiting for more work.
  //        We can still use null job as the "end" signal.
  factor_job* job;
  while ((job = queue_get(iqueue))) {
    job->factors = factor(job->number);
    queue_put(oqueue, job);
    jobcount--;
    if(jobcount == 0) {
      close_input();
    }
  }
  completed++;
  pthread_exit(NULL);
}

void
close_input()
{
  close(iqueue);
}

void
close_output()
{
  close(oqueue);
}
