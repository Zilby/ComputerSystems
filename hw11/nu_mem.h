#ifndef NU_MEM_H
#define NU_MEM_H

#include <stdint.h>

void* hw11_malloc(size_t size);
void* hw11_realloc(void* ptr, size_t size);
void  hw11_free(void* ptr);
void  nu_mem_print_stats();
void  nu_print_free_list();

#endif
