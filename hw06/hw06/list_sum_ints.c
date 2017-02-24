#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "nu_mem.h"

typedef struct icell {
    int num;
    struct icell* next;
} icell;

icell*
cons(int xx, icell* rest)
{
    icell* cell = nu_malloc(sizeof(icell));
    cell->num = xx;
    cell->next = rest;
    return cell;
}

void
free_ilist(icell* xs)
{
    if (xs == 0) {
        return;
    }

    free_ilist(xs->next);
    nu_free(xs);
}

int
sum_upto(int nn)
{
    icell* xs = 0;
    for (int ii = 0; ii < nn; ++ii) {
        xs = cons(ii, xs);
    }

    int sum = 0;
    for (icell* pp = xs; pp != 0; pp = pp->next) {
        sum += pp->num;
    }

    free_ilist(xs);
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
