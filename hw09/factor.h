#ifndef FACTOR_H
#define FACTOR_H

#include <stdint.h>

#include "job.h"

void submit_job(job jj);
job  get_result();

void run_jobs();

void factor(int64_t xx, int64_t* size, int64_t* ys);

int factor_init(int num_procs, int64_t count);
void factor_wait_done();
void factor_cleanup();

#endif
