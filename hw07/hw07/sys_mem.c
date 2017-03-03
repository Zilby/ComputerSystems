
#include <stdlib.h>

void* 
nu_malloc(size_t size)
{
    return malloc(size);
}

void 
nu_free(void* ptr)
{
    free(ptr);
}
