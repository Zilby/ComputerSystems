#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

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

    icell* nn = xs->next;
    nu_free(xs);
    free_ilist(nn);
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

double
get_timef()
{
    const double MILLION = 1000 * 1000;
    struct timeval tt;
    gettimeofday(&tt, 0);
    return ((double)tt.tv_sec) + ((double)tt.tv_usec) / MILLION;
}

int
main(int argc, char* argv[])
{
    assert(argc == 2);
    int nn = atoi(argv[1]);

    double t0 = get_timef();

    int ss = sum_upto(nn);

    for (int ii = 0; ii < 10; ++ii) {
        int s1 = sum_upto(nn);
        assert(s1 == ss);
    }

    double t1 = get_timef();

    printf("Sum from 0 to %d = %d\n", nn - 1, ss);
    printf("Took %.03f seconds.\n", t1 - t0);
    return 0;
}
