#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdatomic.h>

#include "xmalloc.h"
#include "list.h"
#include "ivec.h"


/* HW11 TODO:
 *  - This should call / use your HW06 alloctor,
 *    modified to be safe and efficient for
 *    use in a threaded program.
 */

// structs defined so that we can optimize with our stack

typedef struct num_task_list {
    cell* vals;
    long  steps;
    int   dibs;
    pthread_mutex_t lock;
} num_task_list;

typedef struct num_task_ivec {
    ivec* vals;
    long  steps;
    int   dibs;
    pthread_mutex_t lock;
} num_task_ivec ;

// our free cell struct

typedef struct nu_free_cell {
  int64_t              size;
  struct nu_free_cell* next;
} nu_free_cell;

// size of a chunk
static const int64_t CHUNK_SIZE = 65536;
// size of a cache element
static const int64_t CACHE_SIZE = 1024;
static const int64_t CELL_SIZE  = (int64_t)sizeof(nu_free_cell);
// the greatest number of cells for our cache
static const int64_t SIZE_LIMIT = 500;
// the greatest number of cells for our central list
static const int64_t CENTRAL_SIZE_LIMIT = 100;

// thread exclusive elements for our cache
__thread nu_free_cell* cache = 0;
__thread nu_free_cell* stack = 0;
__thread int64_t max_size = 0;

// global elements for our central list
nu_free_cell* central_list = 0;
_Atomic int64_t central_size = 0;
pthread_mutex_t central_mutex = PTHREAD_MUTEX_INITIALIZER;

// for popping off our stack
nu_free_cell*
pop()
{
  nu_free_cell* temp = stack;
  stack = stack->next;
  return temp; 
}

// for pushing to our stack
void
push(nu_free_cell* c)
{
  if(stack && c) {
    c->next = stack;
  }
  stack = c;
}

// for coalescing our shared list
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

// for coalescing our thread cache
static
void
nu_cache_coalesce()
{
  nu_free_cell* pp = cache;
  int free_chunk = 0;
  while (pp != 0 && pp->next != 0) {
    if (((int64_t)pp) + pp->size == ((int64_t) pp->next)) {
      pp->size += pp->next->size;
      pp->next = pp->next->next;
    }
    pp = pp->next;
  }
}

// inserts a cell into our shared list
static
void
nu_free_list_insert(nu_free_cell* cell)
{
  pthread_mutex_lock(&(central_mutex));
  central_size++; 
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

  if (central_size > CENTRAL_SIZE_LIMIT) {
    central_list = 0;
    central_size = 0;
  }
  
  pthread_mutex_unlock(&(central_mutex));
}

// inserts a cell into our cache
static
void
nu_cache_insert(nu_free_cell* cell)
{
  max_size++;
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

  nu_cache_coalesce();

  // garbage collection
  if (max_size > SIZE_LIMIT) {
    nu_free_cell* temp;
    while(cache != 0) {
      temp = cache;
      cache = cache->next;
      nu_free_list_insert(temp);
    }
    max_size = 0;
  }
}

// gets a cell from our shared list
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
      central_size--;
      return pp;
    }
    prev = &(pp->next);
  }
  pthread_mutex_unlock(&(central_mutex));
  return 0;
}

// gets a cell from our cache
static
nu_free_cell*
cache_get_cell(int64_t size)
{
  nu_free_cell** prev = &(cache);
  for (nu_free_cell* pp = cache; pp != 0; pp = pp->next) {
    if (pp->size >= size) {
      *prev = pp->next;
      max_size--;
      return pp;
    }
    prev = &(pp->next);
  }
  return 0;
}

// makes a new cell
static
nu_free_cell*
make_cell()
{
  void* addr = mmap(0, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  nu_free_cell* cell = (nu_free_cell*) addr; 
  cell->size = CHUNK_SIZE;
  return cell;
}

// allocates a new cell
void*
xmalloc(size_t usize)
{
  // pops a cell off the stack if possible
  
  if((usize == sizeof(num_task_list) || usize == sizeof(num_task_ivec)) && stack) {
    nu_free_cell* cell = pop();
    return ((void*)cell) + sizeof(int64_t);
  }
  
  
  int64_t size = (int64_t) usize;
  
  // space for size
  int64_t alloc_size = size + sizeof(int64_t);

  // space for free cell when returned to list
  if (alloc_size < CELL_SIZE) {
    alloc_size = CELL_SIZE;
  }
  
  nu_free_cell* cell;
  
  if (alloc_size > CACHE_SIZE) {
  
    // TODO: Handle large allocations.
    if (alloc_size > CHUNK_SIZE) {
      void* addr = mmap(0, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
      *((int64_t*)addr) = alloc_size;
      return addr + sizeof(int64_t);
    }

    cell = free_list_get_cell(alloc_size);

  } else {
    cell = cache_get_cell(alloc_size);
    if (!cell) {
      cell = free_list_get_cell(alloc_size);
    } 
  }

  if (!cell) {
    cell = make_cell();
  } 
  
  // Return unused portion to free list.
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

void
xfree(void* addr)
{
  //printf("%d, %d\n", max_size, central_size);
  nu_free_cell* cell = (nu_free_cell*)(addr - sizeof(int64_t));
  int64_t size = cell->size;
  cell->next = 0;

  // pushes a cell to the stack if possible
  
  if(size - sizeof(int64_t) == sizeof(num_task_list) ||
     size - sizeof(int64_t) == sizeof(num_task_ivec)) {
    push(cell);
  } else {
  
  
    if(cell->size > CACHE_SIZE) { 
      if (size > CHUNK_SIZE) {
	munmap((void*) cell, size);
      } else {
	cell->size = size;
	nu_free_list_insert(cell);
      }
    } else {
    
      cell->size = size;
      nu_cache_insert(cell);
    }
  }
}

// our realloc method
void*
xrealloc(void* ptr, size_t size)
{
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
