#ifndef NU_MEM_H
#define NU_MEM_H

#include <stdint.h>

typedef struct free_cell {
  size_t size;
  struct free_cell* next;
  struct free_cell* prev;
  int free;
} free_cell;

void* nu_malloc(size_t size);
void  nu_free(void* ptr);
void  nu_mem_print_stats();
void  nu_print_free_list();
void  split_cell(free_cell* c, size_t usize);
free_cell* coalesce(free_cell* c);
void  assert_ok(long rv, char* call);
int contains(free_cell* c);

#endif
