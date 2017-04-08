

#include <stdio.h>

#include "xmalloc.h"
#include "nu_mem.h"

/* HW11 TODO:
 *  - This should call / use your HW06 alloctor,
 *    modified to be thread-safe.
 */

void*
xmalloc(size_t bytes)
{
  //fprintf(stderr, "TODO: Call HW06 allocator in nu_malloc.c\n");
  return hw11_malloc(bytes);
}

void
xfree(void* ptr)
{
  //fprintf(stderr, "TODO: Call HW06 allocator in nu_malloc.c\n");
  hw11_free(ptr);
}

void*
xrealloc(void* prev, size_t bytes)
{
  //fprintf(stderr, "TODO: Implement realloc with HW06 allocator in nu_malloc.c\n");
  return hw11_realloc(prev, bytes);
}

