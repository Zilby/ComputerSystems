#include <stdint.h>
#include <sys/mman.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "nu_mem.h"

static const int64_t CHUNK_SIZE = 65536;
// size of our cell and its memory size
static const size_t CELL_SIZE = sizeof(free_cell) + sizeof(size_t);

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
static void* free_head;
static void* used_head;

int64_t
nu_free_list_length()
{
  free_cell* c = free_head;
  int64_t i = 0;
  while(c && c->next) {
    ++i;
    c = c->next;
  }
  return i;
}

int64_t
nu_used_list_length()
{
  free_cell* c = used_head;
  int64_t i = 0;
  while(c && c->next) {
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
    free_cell* c = free_head;
    // checks if the first cell is the right size in our free list
    // sorts and checks again if not
    if(!(c && c->free && c->size >= usize + CELL_SIZE)) {
      c = merge_sort(c);
      free_head = c;
    }
    if(c && c->free && c->size >= usize + CELL_SIZE) {
      return allocate_cell(c, usize);
    }
    // if there are no free cells of the right size, make a new one
    void* addr = mmap(0, CHUNK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    assert_ok((long)addr, "mmap");
    ++nu_malloc_chunks;
    c = (free_cell*)addr;
    size_t len = CHUNK_SIZE;
    // put size of memory after cell but before returned memory
    *((size_t*)((char*)addr + sizeof(free_cell))) = len;
    c->size = len;
    c->next = 0;
    if(free_head) {
      free_cell* tmp = free_head;
      c->next = tmp;
      tmp->prev = c;
    }
    free_head = c;
    return allocate_cell(c, usize);
  }
}

// allocates a given free cell for nu_malloc
void* allocate_cell(free_cell* c, size_t usize)
{
  // if cell is too big, split it
  if(c->size > usize + CELL_SIZE + CELL_SIZE) {
    split_cell(c, usize);
  } else {
    free_head = c->next;
  }
  // cell is no longer free
  c->free = 0;
  if(used_head) {
    free_cell* tmp = used_head;
    c->next = tmp;
    tmp->prev = c;
  } 
  c->prev = 0;
  used_head = c; 
  ++nu_malloc_count;
  nu_malloc_bytes += usize;
  return (char*)c + CELL_SIZE;
}


// checks if used list currently contains cell
int
contains(free_cell* c)
{
  free_cell* tmp = used_head;
  while(tmp) {
    if(tmp == c) {
      return 1;
    }
    tmp = tmp->next;
  }
  return 0;
}

void
nu_free(void* addr) 
{
  size_t *mem = (size_t*)addr;
  size_t len;
  // get size of memory
  mem--;
  len = *mem;
  // try making addr a cell
  free_cell* c;
  char *tmp;
  tmp = addr;
  addr = tmp -= CELL_SIZE;
  c = (free_cell*)addr;
  // Free large blocks with munmap.
  if(len >= (size_t)CHUNK_SIZE) {
    // if it's a cell remove from the list
    if(contains(c)) {
      if(c->prev) {
	c->prev->next = c->next;
      }
      if(c->next) {
	c->next->prev = c->prev;
      }
      if(c == used_head) {
	  used_head = c->next;
      }
    }
    long rv = munmap((void*)mem, len);
    assert_ok(rv, "munmap");
    ++nu_free_chunks;
    ++nu_free_count;
    nu_free_bytes += len;
    return;
  }
  // if it's not large memory treat it like a cell
  c->free = 1;
  if(c->next) {
    c->next->prev = c->prev;
  }
  if(c->prev) {
    c->prev->next = c->next;
  }
  if(c == used_head) {
    used_head = c->next;
  }
  c->next = 0;
  c->prev = 0;
  if(free_head) {
    free_cell* tmp = free_head;
    c->next = tmp;
    tmp->prev = c;
  }
  free_head = c;
  ++nu_free_count;
  nu_free_bytes += c->size;
  free_cell* n;
  // iterate through our free cells to see if we can coalesce
  if(c->next) {
    n = c->next;
    while(n) {
      if(n->free && ((char*)c + c->size == (char*)n ||
		     (char*)c == (char*)n + n->size)) {
	c = coalesce(c, n);
      }
      n = n->next;
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
  size_t len = c->size - usize - CELL_SIZE;
  *((size_t*)((char*)split + sizeof(free_cell))) = len;
  split->size = len;
  split->free = 1;
  split->prev = 0;
  // because c is already the free_head here, make split the head
  split->next = c->next;
  if(c->next) {
    c->next->prev = split;
  }
  free_head = split;
  *((size_t*)((char*)c + sizeof(free_cell))) = usize + CELL_SIZE;
  c->size = usize + CELL_SIZE;
}

// coalesces adjacent cells (assumes cells are adjacent, but does not
// assume which one is adjacent to which)
free_cell*
coalesce(free_cell* c, free_cell* n)
{
  if((char*)c + c->size == (char*)n) {
    c->size += n->size;
    *((char*)c + sizeof(free_cell)) = c->size;
    if(n->prev) {
      n->prev->next = n->next;
    }
    if(n->next) {
      n->next->prev = n->prev;
    }
    if(n == free_head) {
      free_head = n->next;
    }
    return c;
  } else {
    n->size += c->size;
    *((char*)n + sizeof(free_cell)) = n->size;
    if(c->prev) {
      c->prev->next = c->next;
    }
    if(c->next) {
      c->next->prev = c->prev;
    }
    if(c == free_head) {
      free_head = c->next;
    }
    return n;
  }
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

// basic merge sorting function
free_cell* merge_sort(free_cell* header)
{
  if (!header || !(header->next)) {
    return header;
  }
  free_cell* second = split(header);
  header = merge_sort(header);
  second = merge_sort(second);
  
  return merge(header,second);
}


// merging function for our sort
free_cell* merge(free_cell* first, free_cell* second)
{
  if (!first) {
        return second;
  } 
  if (!second) {
        return first;
  }
  if(first->size > second->size) {
    first->next = merge(first->next,second);
    first->next->prev = first;
    first->prev = NULL;
    return first;
  }
  else {
    second->next = merge(first,second->next);
    second->next->prev = second;
    second->prev = NULL;
    return second;
  }
}

// splits the list
free_cell* split(free_cell* header)
{
  free_cell* fast = header;
  free_cell* slow = header;
  while (fast->next && fast->next->next) {
    fast = fast->next->next;
    slow = slow->next;
  }
  free_cell* tmp = slow->next;
  slow->next = NULL;
  return tmp;
}
