#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "queue.h"

pthread_mutex_t mutex;
pthread_cond_t  condv;

queue* 
make_queue()
{
  queue* qq = malloc(sizeof(queue));
  qq->head = 0;
  qq->tail = 0;
  qq->flag = 0;
  return qq;
}

void 
free_queue(queue* qq)
{
  pthread_mutex_lock(&mutex);
  assert(qq->head == 0 && qq->tail == 0);
  free(qq);
  pthread_mutex_unlock(&mutex);
}

void 
queue_put(queue* qq, void* msg)
{
  pthread_mutex_lock(&mutex);
  qnode* node = malloc(sizeof(qnode));
  node->data = msg;
  node->prev = 0;
  node->next = 0;
    
  node->next = qq->head;
  qq->head = node;

  if (node->next) {
    node->next->prev = node;
  } 
  else {
    qq->tail = node;
  }
  pthread_cond_signal(&condv);
  pthread_mutex_unlock(&mutex);
}

void* 
queue_get(queue* qq)
{
  pthread_mutex_lock(&mutex);
  while (!qq->tail && !(qq->flag)) {
    pthread_cond_wait(&condv, &mutex);
  }
  if(qq->flag) {
    pthread_mutex_unlock(&mutex);
    return 0;
  }
  qnode* node = qq->tail;

  if (node->prev) {
    qq->tail = node->prev;
    node->prev->next = 0;
  }
  else {
    qq->head = 0;
    qq->tail = 0;
  }

  void* msg = node->data;
  free(node);
  pthread_mutex_unlock(&mutex);
  return msg;
}

void
close(queue* qq) {
  pthread_mutex_lock(&mutex);
  qq->flag = 1;
  pthread_cond_broadcast(&condv);
  pthread_mutex_unlock(&mutex);
}

