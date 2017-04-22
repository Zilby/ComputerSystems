
#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "directory.h"

void directory_init() {
  dirent* ents = (dirent*) pages_get_page(1);
  ents[0].pnum = 1; 
  ents[0].node = (pnode*) pages_get_node(0);
  ents[0].node->refs = 1;
  ents[0].node->size = 4096;
  ents[0].node->mode = 0040755;
  strcpy(ents[0].name, "/");
}

directory directory_from_pnum(int pnum) {
  dirent* ents = (dirent*) pages_get_page(pnum);
  directory* d = malloc(sizeof(directory));
  d->pnum = pnum;
  d->ents = ents;
  d->node = (pnode*) pages_get_node(pnum);
  return *d;
}

int directory_lookup_pnum(directory dd, const char* name) {
  dirent* ents = dd.ents;
  for(int i = 0; i < DIR_SIZE; i++) {
    if(strcmp(ents[i].name, name) == 0) {
      return ents[i].pnum;
    }
  }
  return -1; 
}

int tree_lookup_pnum(const char* path) {
  return 0;
}

directory directory_from_path(const char* path) {
  directory* d = malloc(sizeof(directory));
  slist* s = directory_list(path);
  s = s->next;
  d->pnum = 1;
  d->ents = (dirent*) pages_get_page(1);
  d->node = (pnode*) pages_get_node(0);
  while(s && s->next != 0) {
    int found = 0;
    for(int i = 1; i < DIR_SIZE; i++) {
      if(strcmp(d->ents[i].name, s->data) == 0) {
	d->pnum = d->ents[i].pnum;
	d->ents = (dirent*) pages_get_page(d->pnum);
	d->node = (pnode*) pages_get_node(d->pnum);
	strcpy(d->ents[0].name, s->data);
	d->ents[0].pnum = d->pnum; 
	d->ents[0].node = (pnode*) pages_get_node(d->pnum);
	s = s->next;
	found = 1;
	break;
      }
    }
    if(!found) {
      return *d;
    }
  }
  return *d;
}

int directory_put_ent(directory dd, const char* name, mode_t mode) {
  dirent* ents = dd.ents;
  for(int i = 1; i < DIR_SIZE; i++) {
    if(!ents[i].node || ents[i].node->refs == 0) {
      if(!ents[i].node) {
	int j = 0;
	pnode* p = pages_get_node(j);
	while(!p || p->refs != 0) {
	  p = pages_get_node(++j);
	}
	ents[i].node = p;
      }
      
      strcpy(ents[i].name, name);
      ents[i].pnum = pages_find_empty();
      ents[i].node->refs = 1;
      ents[i].node->mode = mode;
      ents[i].node->size = strlen(name);
      return 0;
    }
  }
  return -1;
}

int directory_delete(directory dd, const char* name) {
  dirent* ents = dd.ents;
  for(int i = 1; i < DIR_SIZE; i++) {
    if(strcmp(ents[i].name, name) == 0) {
      ents[i].node->refs = 0;
      ents[i].node = 0;
      return 0;
    }
  }
  return -1;
}

slist* directory_list(const char* path) {
  if(strcmp(path, "/") == 0) {
    slist* s = malloc(sizeof(slist*));
    s->refs = 1;
    s->data = strdup(path);
    s->next = 0;
    return s;
  }
  return s_split(path, '/');
}

void print_directory(directory dd) {
  dirent* ents = dd.ents;
  for(int i = 1; i < DIR_SIZE; i++) {
    if(ents[i].node->refs == 0) {
      printf("%s", strcat(ents[i].name, " "));
    }
  }
}
