#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "nu_mem.h"
#include "trand.h"

int
main(int _ac, char* _av[])
{
    const int ASIZE = 25000;

    int** arrays = nu_malloc(sizeof(int**) * ASIZE);
    int*  sizes  = nu_malloc(sizeof(int)   * ASIZE);
        
    int64_t foo = 0;

    for (int ii = 0; ii < ASIZE; ++ii) {
        int nn = trand_range(1, 1024);
        if (ii % 1000 == 0) {
            nn = ASIZE;
        }
        sizes[ii] = nn;

        if (ii > 0 && ii % 2 == 0) {
            nu_free(arrays[ii / 2]);
            arrays[ii / 2] = 0;
        }

        arrays[ii] = nu_malloc(nn * sizeof(int));
        
        for (int jj = 0; jj < nn; ++jj) {
            arrays[ii][jj] = jj + ii;
        }

        for (int jj = 0; jj < nn; ++jj) {
            foo += arrays[ii][jj];
            foo = foo - jj;
        }
    }

    for (int ii = 0; ii < ASIZE; ++ii) {
        if (arrays[ii]) {
            nu_free(arrays[ii]);
        }
    }

    nu_free(arrays);
    nu_free(sizes);

    printf("foo = %ld\n", foo);
    nu_mem_print_stats();
    return 0;
}
