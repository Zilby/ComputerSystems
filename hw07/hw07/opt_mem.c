#include <stdlib.h>
#include <stdio.h>
#include "hw06_mem.h"

typedef struct icell {
    int num;
    struct icell* next;
} icell;

typedef struct nu_free_cell {
    int64_t              size;
    struct nu_free_cell* next;
} nu_free_cell;

static nu_free_cell* stack = 0;


// for popping off our stack
nu_free_cell*
pop()
{
  if(stack) {
    nu_free_cell* temp = stack;
    stack = stack->next;
    return temp; 
  } else {
    return stack;
  }
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

void* 
nu_malloc(size_t size)
{
  if(size == sizeof(struct icell) && stack) {
    nu_free_cell* cell = pop();
    return ((void*)cell) + sizeof(int64_t);
  }
  return hw06_malloc(size);
}

void 
nu_free(void* ptr)
{
  nu_free_cell* cell = (nu_free_cell*)(ptr - sizeof(int64_t));
  int64_t size = *((int64_t*) cell);
  if(size - sizeof(int64_t) == sizeof(struct icell)) {
    push(cell); 
  } else {
    hw06_free(ptr);
  }
}

