
#include <stdlib.h>

#include "hw06_mem.h"

void* 
nu_malloc(size_t size)
{
    return hw06_malloc(size);
}

void 
nu_free(void* ptr)
{
    hw06_free(ptr);
}
