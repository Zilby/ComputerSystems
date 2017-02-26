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
void* allocate_cell(free_cell* c, size_t usize);
void  nu_free(void* ptr);
void  nu_mem_print_stats();
void  nu_print_free_list();
void  split_cell(free_cell* c, size_t usize);
free_cell* coalesce(free_cell* c, free_cell* n);
void  assert_ok(long rv, char* call);
int contains(free_cell* c);
free_cell* merge(free_cell* first, free_cell* second);
free_cell* split(free_cell* head);
free_cell* merge_sort(free_cell* header);

#endif
