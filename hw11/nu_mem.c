#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "nu_mem.h"

pthread_mutex_t mutex;
pthread_cond_t  condv;

typedef struct nu_free_cell {
  int64_t              size;
  struct nu_free_cell* next;
} nu_free_cell;

static const int64_t CHUNK_SIZE = 65536;
static const int64_t CELL_SIZE  = (int64_t)sizeof(nu_free_cell);

static nu_free_cell* nu_free_list = 0;

static int64_t nu_malloc_count  = 0; // How many times has malloc returned a block.
static int64_t nu_malloc_bytes  = 0; // How many bytes have been allocated total
static int64_t nu_free_count    = 0; // How many times has free recovered a block.
static int64_t nu_free_bytes    = 0; // How many bytes have been recovered total.
static int64_t nu_malloc_chunks = 0; // How many chunks have been mmapped?
static int64_t nu_free_chunks   = 0; // How many chunks have been munmapped?

int64_t
nu_free_list_length()
{
  pthread_mutex_lock(&mutex);
  int len = 0;
  for (nu_free_cell* pp = nu_free_list; pp != 0; pp = pp->next) {
    len++;
  }
  pthread_mutex_unlock(&mutex);
  return len;
}

void
nu_print_free_list()
{
  pthread_mutex_lock(&mutex);
  nu_free_cell* pp = nu_free_list;
  printf("= Free list: =\n");

  for (; pp != 0; pp = pp->next) {
    printf("%lx: (cell %ld %lx)\n", (int64_t) pp, pp->size, (int64_t) pp->next); 
  }
  pthread_mutex_unlock(&mutex);
}

void
nu_mem_print_stats()
{
  fprintf(stderr, "\n== nu_mem stats ==\n");
  fprintf(stderr, "malloc count: %ld\n", nu_malloc_count);
  fprintf(stderr, "malloc bytes: %ld\n", nu_malloc_bytes);
  fprintf(stderr, "free count: %ld\n", nu_free_count);
  fprintf(stderr, "free bytes: %ld\n", nu_free_bytes);
  fprintf(stderr, "malloc chunks: %ld\n", nu_malloc_chunks);
  fprintf(stderr, "free chunks: %ld\n", nu_free_chunks);
  fprintf(stderr, "free list length: %ld\n", nu_free_list_length());
}

static
void
nu_free_list_coalesce()
{
  nu_free_cell* pp = nu_free_list;
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
  if (nu_free_list == 0 || ((uint64_t) nu_free_list) > ((uint64_t) cell)) {
    cell->next = nu_free_list;
    nu_free_list = cell;
    return;
  }

  nu_free_cell* pp = nu_free_list;

  while (pp->next != 0 && ((uint64_t)pp->next) < ((uint64_t) cell)) {
    pp = pp->next;
  }

  cell->next = pp->next;
  pp->next = cell;

  nu_free_list_coalesce();
}

static
nu_free_cell*
free_list_get_cell(int64_t size)
{
  nu_free_cell** prev = &nu_free_list;

  for (nu_free_cell* pp = nu_free_list; pp != 0; pp = pp->next) {
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
  nu_malloc_chunks += 1;
  cell->size = CHUNK_SIZE;
  return cell;
}

void*
hw11_malloc(size_t usize)
{
  pthread_mutex_lock(&mutex);
  int64_t size = (int64_t) usize;

  // space for size
  int64_t alloc_size = size + sizeof(int64_t);

  // space for free cell when returned to list
  if (alloc_size < CELL_SIZE) {
    alloc_size = CELL_SIZE;
  }

  nu_malloc_count += 1;
  nu_malloc_bytes += alloc_size;

  // TODO: Handle large allocations.
  if (alloc_size > CHUNK_SIZE) {
    void* addr = mmap(0, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    *((int64_t*)addr) = alloc_size;
    nu_malloc_chunks += 1;
    pthread_mutex_unlock(&mutex);
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
  pthread_mutex_unlock(&mutex);
  return ((void*)cell) + sizeof(int64_t);
}

void
hw11_free(void* addr) 
{
  pthread_mutex_lock(&mutex);
  nu_free_cell* cell = (nu_free_cell*)(addr - sizeof(int64_t));
  int64_t size = *((int64_t*) cell);

  if (size > CHUNK_SIZE) {
    nu_free_chunks += 1;
    munmap((void*) cell, size);
  }
  else {
    cell->size = size;
    nu_free_list_insert(cell);
  }

  nu_free_count += 1;
  nu_free_bytes += size;
  pthread_mutex_unlock(&mutex);
}

void*
hw11_realloc(void* ptr, size_t size)
{
  void* new_ptr;
  if(size > 0) {
    new_ptr = hw11_malloc(size);
    pthread_mutex_lock(&mutex);
    nu_free_cell* cell = (nu_free_cell*)(ptr - sizeof(int64_t));
    int64_t size_old = *((int64_t*) cell);
    memcpy(new_ptr, ptr, size_old);
    pthread_mutex_unlock(&mutex);
  }
  hw11_free(ptr);
  return new_ptr;
}
