#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job.h"
#include "factor.h"

int
main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("Usage:\n");
        printf("  ./main procs start count\n");
        return 1;
    }

    int procs = atoi(argv[1]);

    int64_t start = atol(argv[2]);
    int64_t count = atol(argv[3]);

    int pid = factor_init(procs, count);
    if(pid == 1) {
      return 0;
    }

    for (int64_t ii = 0; ii < count; ++ii) {
        job jj = make_job(start + ii);
        submit_job(jj);
    }

    factor_cleanup();

    return 0;
}
