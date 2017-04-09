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
  struct nu_free_cell* next;
} nu_free_cell;

static const int64_t CHUNK_SIZE = 65536;
static const int64_t CACHE_SIZE = 128;
static const int64_t CELL_SIZE  = (int64_t)sizeof(nu_free_cell);
static const int64_t SIZE_LIMIT = 50;

_Atomic int64_t current_id = 0;

__thread int64_t id = 0;
__thread nu_free_cell* cache = 0;
__thread int64_t max_size = 0;

nu_free_cell* central_list = 0;
int64_t central_size = 0;
pthread_mutex_t central_mutex = PTHREAD_MUTEX_INITIALIZER;

int counter = 0; // for test purposes

static
void
nu_free_list_coalesce()
{
  nu_free_cell* pp = central_list;
  int free_chunk = 0;
  while (pp != 0 && pp->next != 0) {
    if (((int64_t)pp) + pp->size == ((int64_t) pp->next)) {
      pp->size += pp->next->size;
      pp->next = pp->next->next;
    }
    pp = pp->next;
  }
}

static
void
nu_free_list_insert(nu_free_cell* cell)
{
  pthread_mutex_lock(&(central_mutex));
    
  if (central_list == 0 || ((uint64_t) central_list) > ((uint64_t) cell)) {
    cell->next = central_list;
    central_list = cell;
    pthread_mutex_unlock(&(central_mutex));
    return;
  }

  nu_free_cell* pp = central_list;

  while (pp->next != 0 && ((uint64_t)pp->next) < ((uint64_t) cell)) {
    pp = pp->next;
  }

  cell->next = pp->next;
  pp->next = cell;
  
  nu_free_list_coalesce();
  pthread_mutex_unlock(&(central_mutex));
}

static
void
nu_cache_insert(nu_free_cell* cell)
{    
  if (cache == 0 || ((uint64_t) cache) > ((uint64_t) cell)) {
    cell->next = cache;
    cache = cell;
    return;
  }

  nu_free_cell* pp = cache;

  while (pp->next != 0 && ((uint64_t)pp->next) < ((uint64_t) cell)) {
    pp = pp->next;
  }

  cell->next = pp->next;
  pp->next = cell;
}

static
nu_free_cell*
free_list_get_cell(int64_t size)
{
  pthread_mutex_lock(&(central_mutex));
  nu_free_cell** prev = &(central_list);
  for (nu_free_cell* pp = central_list; pp != 0; pp = pp->next) {
    if (pp->size >= size) {
      *prev = pp->next;
      pthread_mutex_unlock(&(central_mutex));
      return pp;
    }
    prev = &(pp->next);
  }
  pthread_mutex_unlock(&(central_mutex));
  return 0;
}

static
nu_free_cell*
cache_get_cell(int64_t size)
{
  nu_free_cell** prev = &(cache);
  for (nu_free_cell* pp = cache; pp != 0; pp = pp->next) {
    if (pp->size >= size) {
      *prev = pp->next;
      return pp;
    }
    prev = &(pp->next);
  }
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
  int64_t size = (int64_t) usize;
  
  // space for size
  int64_t alloc_size = size + sizeof(int64_t);

  // space for free cell when returned to list
  if (alloc_size < CELL_SIZE) {
    alloc_size = CELL_SIZE;
  }

  if (alloc_size > CACHE_SIZE) {
  
    // TODO: Handle large allocations.
    if (alloc_size > CHUNK_SIZE) {
      void* addr = mmap(0, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      *((int64_t*)addr) = alloc_size;
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
      *((int64_t*)cell) = alloc_size;
    }
    return ((void*)cell) + sizeof(int64_t);
  } else {
    nu_free_cell* cell = cache_get_cell(alloc_size);
    if (!cell) {
      cell = free_list_get_cell(alloc_size);
      if (!cell) {
	cell = make_cell();
      }
    } else {
      max_size--;
    }
    
    int64_t rest_size = cell->size - alloc_size;
  
    if (rest_size >= CELL_SIZE) {
      void* addr = (void*) cell;
      nu_free_cell* rest = (nu_free_cell*) (addr + alloc_size);
      rest->size = rest_size;
      nu_cache_insert(rest);
      *((int64_t*)cell) = alloc_size;
    }
    return ((void*)cell) + sizeof(int64_t);
  }
}

void
xfree(void* addr)
{
  //fprintf(stderr, "TODO: Implement parallel allocator in par_malloc.c\n");
  nu_free_cell* cell = (nu_free_cell*)(addr - sizeof(int64_t));
  int64_t size = cell->size;
  if(cell->size > CACHE_SIZE || max_size > SIZE_LIMIT) {    
    if (size > CHUNK_SIZE) {
      munmap((void*) cell, size);
    }
    else {
      cell->size = size;
      nu_free_list_insert(cell);
    }
  } else {
    cell->size = size;
    nu_cache_insert(cell);
    max_size++;
  }
}

void*
xrealloc(void* ptr, size_t size)
{
  //fprintf(stderr, "TODO: Implement parallel allocator in par_malloc.c\n");
  void* new_ptr;
  if(size > 0) {
    new_ptr = xmalloc(size);
    nu_free_cell* cell = (nu_free_cell*)(ptr - sizeof(int64_t));
    int64_t size_old = *((int64_t*) cell);
    memcpy(new_ptr, ptr, size_old);
  }
  xfree(ptr);
  return new_ptr;
}
