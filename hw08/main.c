#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "int128.h"
#include "factor.h"
#include "ivec.h"

int
main(int argc, char* argv[])
{
  if (argc != 4) {
    printf("Usage:\n");
    printf("  ./main threads start count\n");
    return 1;
  }

  int threads = atoi(argv[1]);

  int128_t start = atoh(argv[2]);
  int64_t  count = atol(argv[3]);
    
  // spawning threads in init
  factor_init(threads, count);
    
  for (int64_t ii = 0; ii < count; ++ii) {
    factor_job* job = make_job(start + ii);
    submit_job(job);
  }
    
  int64_t oks = 0;
  factor_job* job;
  
  while((job = get_result())) {

    print_int128(job->number);
    printf(": ");
    print_ivec(job->factors);

    ivec* ys = job->factors;
        
    int128_t prod = 1;
    for (int ii = 0; ii < ys->len; ++ii) {
      prod *= ys->data[ii];
    }
      
    if (prod == job->number) {
      ++oks;
    }
    else {
      fprintf(stderr, "Warning: bad factorization");
    }
    free_job(job);
  }

  printf("Factored %ld / %ld numbers.\n", oks, count);

  // threads are joined in cleanup
  factor_cleanup();

  return 0;
}
