#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>

#include "xmalloc.h"

/* HW11 TODO:
 *  - This should call / use your HW06 alloctor,
 *    modified to be safe and efficient for
 *    use in a threaded program.
 */

typedef struct nu_free_cell {
  int64_t              size;
  int64_t              l_id;
  struct nu_free_cell* next;
} nu_free_cell;

static const int64_t CHUNK_SIZE = 65536;
static const int64_t CELL_SIZE  = (int64_t)sizeof(nu_free_cell);

_Atomic int64_t current_id = 0;

__thread int64_t id = 0;
__thread nu_free_cell* list = 0;
__thread pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static nu_free_cell* nu_free_lists[10000000];
static pthread_mutex_t mutexes[10000000];

void
init() {
  if(id == 0) {
    ++current_id;
    id = current_id;
    nu_free_lists[id] = list;
    mutexes[id] = mutex;
  }
}

static
void
nu_free_list_coalesce()
{
  nu_free_cell* pp = list;
  int free_chunk = 0;

  while (pp != 0 && pp->next != 0) {
    if (((int64_t)pp) + pp->size == ((int64_t) pp->next)) {
      pp->size += pp->next->size;
      pp->next  = pp->next->next;
    }

    pp = pp->next;
  }
}

static
void
nu_free_list_insert(nu_free_cell* cell)
{
  pthread_mutex_lock(&(mutex));
  if (list == 0 || ((uint64_t) list) > ((uint64_t) cell)) {
    cell->next = list;
    list = cell;
    pthread_mutex_unlock(&(mutex));
    return;
  }

  nu_free_cell* pp = list;

  while (pp->next != 0 && ((uint64_t)pp->next) < ((uint64_t) cell)) {
    pp = pp->next;
  }

  cell->next = pp->next;
  pp->next = cell;

  nu_free_list_coalesce();
  pthread_mutex_unlock(&(mutex));
}

static
nu_free_cell*
free_list_get_cell(int64_t size)
{
  pthread_mutex_lock(&(mutex));
  nu_free_cell** prev = &(list);

  for (nu_free_cell* pp = list; pp != 0; pp = pp->next) {
    if (pp->size >= size) {
      *prev = pp->next;
      pthread_mutex_unlock(&(mutex));
      return pp;
    }
    prev = &(pp->next);
  }
  pthread_mutex_unlock(&(mutex));
  return 0;
}

static
nu_free_cell*
make_cell()
{
  void* addr = mmap(0, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  nu_free_cell* cell = (nu_free_cell*) addr; 
  cell->size = CHUNK_SIZE;
  return cell;
}

void*
xmalloc(size_t usize)
{
  //fprintf(stderr, "TODO: Implement parallel allocator in par_malloc.c\n");
  //pthread_mutex_lock(&(list->mutex));
  //mutexes[current_id] = mutex;
  //++current_id;
  
  int64_t size = (int64_t) usize;

  // space for size
  int64_t alloc_size = size + sizeof(int64_t);

  // space for free cell when returned to list
  if (alloc_size < CELL_SIZE) {
    alloc_size = CELL_SIZE;
  }


  // TODO: Handle large allocations.
  if (alloc_size > CHUNK_SIZE) {
    void* addr = mmap(0, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    *((int64_t*)addr) = alloc_size;
    //pthread_mutex_unlock(&(list->mutex));
    return addr + sizeof(int64_t);
  }

  nu_free_cell* cell = free_list_get_cell(alloc_size);
  if (!cell) {
    cell = make_cell();
  }

  // Return unused portion to free list.
  int64_t rest_size = cell->size - alloc_size;
  if (rest_size >= CELL_SIZE) {
    void* addr = (void*) cell;
    nu_free_cell* rest = (nu_free_cell*) (addr + alloc_size);
    rest->size = rest_size;
    nu_free_list_insert(rest);
  }

  *((int64_t*)cell) = alloc_size;
  //pthread_mutex_unlock(&(list->mutex));
  return ((void*)cell) + sizeof(int64_t);
}

void
xfree(void* addr)
{
  //fprintf(stderr, "TODO: Implement parallel allocator in par_malloc.c\n");
  //pthread_mutex_lock(&(list->mutex));
  // initialise our list to the list of lists
  init();
  pthread_mutex_lock(&(mutexes[id]));
  nu_free_cell* cell = (nu_free_cell*)(addr - sizeof(int64_t));
  int64_t size = *((int64_t*) cell);

  if (size > CHUNK_SIZE) {
    munmap((void*) cell, size);
  }
  else {
    cell->size = size;
    nu_free_list_insert(cell);
  }

  //pthread_mutex_unlock(&(list->mutex));
  pthread_mutex_unlock(&(mutexes[id]));
}

void*
xrealloc(void* ptr, size_t size)
{
  //fprintf(stderr, "TODO: Implement parallel allocator in par_malloc.c\n");
  init();
  void* new_ptr;
  if(size > 0) {
    new_ptr = xmalloc(size);
    pthread_mutex_lock(&(mutexes[id]));
    nu_free_cell* cell = (nu_free_cell*)(ptr - sizeof(int64_t));
    int64_t size_old = *((int64_t*) cell);
    memcpy(new_ptr, ptr, size_old);
    //pthread_mutex_unlock(&(list->mutex));
    pthread_mutex_unlock(&(mutexes[id]));
  }
  xfree(ptr);
  return new_ptr;
}
