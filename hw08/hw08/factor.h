#ifndef FACTOR_H
#define FACTOR_H

#include <stdint.h>

#include "int128.h"
#include "ivec.h"

pthread_mutex_t mutex;
pthread_cond_t  condv;

typedef struct factor_job {
    int128_t number;
    ivec*    factors;
} factor_job;

factor_job* make_job(int128_t nn);
void free_job(factor_job* job);

void submit_job(factor_job* job);
factor_job* get_result();

void* run_jobs(void* arg);

ivec* factor(int128_t xx);

void factor_init(int i, int j);
void factor_cleanup();
void close_input();
void close_output();

#endif
