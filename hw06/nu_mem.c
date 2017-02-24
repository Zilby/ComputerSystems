#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "nu_mem.h"

static const int64_t CHUNK_SIZE = 65536;
// size of our cell struct
static const int64_t CELL_SIZE = (int64_t)sizeof(free_cell);

// You should update these counters on memory allocation / deallocation events.
// These counters should only go up, and should provide totals for the entire
// execution of the program.
static int64_t nu_malloc_count  = 0; // How many times has malloc returned a block.
static int64_t nu_malloc_bytes  = 0; // How many bytes have been allocated total
static int64_t nu_free_count    = 0; // How many times has free recovered a block.
static int64_t nu_free_bytes    = 0; // How many bytes have been recovered total.
static int64_t nu_malloc_chunks = 0; // How many chunks have been mmapped?
static int64_t nu_free_chunks   = 0; // How many chunks have been munmapped?

// the start of our linked list
static void* head;

int64_t
nu_free_list_length()
{
  free_cell* c = head;
  int64_t i = 0;
  while(c) {
    ++i;
    c = c->next;
  }
  return i;
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

void*
nu_malloc(size_t usize)
{
  // Allocate large blocks (>= 64k) of memory directly with
  // mmap.
  if(usize >= CHUNK_SIZE) {
    size_t* addr;
    size_t len = usize + sizeof(usize);
    addr = mmap(0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    assert_ok((long)addr, "mmap");
    ++nu_malloc_chunks;
    // puts size of memory in front
    *addr = len;
    ++nu_malloc_count;
    nu_malloc_bytes += usize;
    return (void*)(&addr[1]);
  }
  // Allocate small blocks of memory by allocating 64k chunks
  // and then satisfying multiple requests from that.
  else {
    free_cell* c = head;
    free_cell* p = 0; // for keeping track of previous in case we need to allocate a new cell
    // checks if there are any free cells of the right size in our free list
    while(c) {
      if(c->free && c->size >= usize) {
	// if cell is too big, split it
	if(c->size > usize + CELL_SIZE) {
	  split_cell(c, usize);
	}
	// cell is no longer free
	c->free = 0;
	++nu_malloc_count;
	nu_malloc_bytes += usize;
	return (char*)c + CELL_SIZE;
      }
      p = c;
      c = c->next;
    }
    // if there are no free cells of the right size, make a new one
    void* addr = mmap(0, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    assert_ok((long)addr, "mmap");
    ++nu_malloc_chunks;
    c = (free_cell*)addr;
    c->size = CHUNK_SIZE - CELL_SIZE;
    c->next = 0;
    c->prev = p;
    // assign value to head if unassigned or previous cell
    if(head) {
      p->next = c;
    } else {
      head = c;
    }
    // if there's extra room split the cell
    if(c->size > usize + CELL_SIZE) {
      split_cell(c, usize);
    }
    c->free = 0;
    ++nu_malloc_count;
    nu_malloc_bytes += usize;
    return (char*)c + CELL_SIZE;
  }
}

// checks if list currently contains cell
int
contains(free_cell* c)
{
  free_cell* c2 = head;
  while(c2) {
    if(c2 == c) {
      return 1;
    }
    c2 = c2->next;
  }
  return 0;
}

void
nu_free(void* addr) 
{
  size_t *mem = (size_t*)addr;
  size_t len;
  // get size of memory in case needed
  mem--;
  len = *mem;
  // try making addr a cell
  free_cell* c;
  char *tmp;
  tmp = addr;
  addr = tmp -= CELL_SIZE;
  c = (free_cell*)addr;
  printf("here\n");
  // if it's not a cell
  if(!contains(c)) {
    // Free large blocks with munmap.
    long rv = munmap((void*)mem, len);
    assert_ok(rv, "munmap");
    ++nu_free_chunks;
    ++nu_free_count;
    nu_free_bytes += len;
  }
  // Free small blocks by saving them for reuse.
  //   - Stick together adjacent small blocks into bigger blocks.
  //   - Advanced: If too many full chunks have been freed (> 4 maybe?)
  //     return some of them with munmap.
  else {
    c->free = 1;
    ++nu_free_count;
    nu_free_bytes += c->size; 
    // try coalesce with previous
    while(c->prev && c->prev->free
	  && ((char*)c->prev + CELL_SIZE + c->prev->size == (char*)c ||
	      (char*)c->prev == (char*)c + CELL_SIZE + c->size)) {
      c = coalesce(c->prev);
    }
    // try coalesce with next
    while(c->next && c->next->free
	  && ((char*)c + CELL_SIZE + c->size == (char*)c->next ||
	      (char*)c == (char*)c->next + CELL_SIZE + c->next->size)) {
      c = coalesce(c);
    }
    // munmap excess large cells
    if(c->size >= CHUNK_SIZE) {
      free_cell* p = c->prev;
      free_cell* n = c->next;
      if(p) {
	p->next = n;
      }
      if(n) {
	n->prev = p;
      }
      long rv = munmap((void*)c, c->size + CELL_SIZE);
      assert_ok(rv, "unmap");
    }  
      
  }  
}

// splits a free cell into two
void
split_cell(struct free_cell* c, size_t usize)
{
  free_cell* split;
  split = (free_cell*)((char*)c + usize + CELL_SIZE);
  // puts size of memory in front
  split->size = c->size - usize - CELL_SIZE;
  split->free = 1;
  split->next = c->next;
  split->prev = c;
  c->size = usize;
  c->next = split;
}

// coalesces adjacent cells (assumes cells are adjacent, and given
// a cell adjacent to its next) 
free_cell*
coalesce(struct free_cell* c)
{
  if((char*)c + CELL_SIZE + c->size == (char*)c->next) {
    c->size += CELL_SIZE + c->next->size;
    c->next = c->next->next;
    if(c->next) {
      c->next->prev = c;
    }
    return c;
  } else {
    c->next->size += CELL_SIZE + c->size;
    c->next->prev = c->prev;
    if(c->prev) {
      c->prev->next = c->next;
    }
    return c->next;
  }
  return c;
}

// for checking if mmap and munmap worked
void
assert_ok(long rv, char* call)
{
  if(rv == -1) {
    fprintf(stderr, "Failed call: %s\n", call);
    perror("Error:");
    exit(1);
  }
}
