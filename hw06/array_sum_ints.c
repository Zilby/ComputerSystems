#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "nu_mem.h"

int
sum_upto(int nn)
{
    int* xs = nu_malloc(nn * sizeof(int));
    
    for (int ii = 0; ii < nn; ++ii) {
        xs[ii] = ii;
    }

    int sum = 0;
    for (int ii = 0; ii < nn; ++ii) {
        sum += xs[ii];
     }
    
    nu_free(xs);
    return sum;
}

int
main(int argc, char* argv[])
{
    assert(argc == 2);
    int nn = atoi(argv[1]);
    int s0 = sum_upto(nn);
    printf("Sum from 0 to %d = %d\n", nn - 1, s0);
    int s1 = sum_upto(nn);
    printf("Sum from 0 to %d = %d\n", nn - 1, s1);

    nu_mem_print_stats();
    return 0;
}
