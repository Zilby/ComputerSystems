
#include "job.h"

job
make_job(int64_t nn)
{
    job jj;
    jj.number = nn;
    jj.count  = 0;
    return jj;
}

