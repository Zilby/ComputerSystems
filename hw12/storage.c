
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage.h"
#include "pages.h"
#include "directory.h"

void
storage_init(const char* path)
{
  pages_init(path);
  directory_init();
}

static int
streq(const char* aa, const char* bb)
{
  return strcmp(aa, bb) == 0;
}


static dirent*
get_dirent(const char* path) {
  directory d = directory_from_path(path);
  slist* s = directory_list(path);
  while(s->next != 0) {
    s = s->next;
  }
  for(int i = 0; i < DIR_SIZE; i++) {
    if(strcmp(d.ents[i].name, s->data) == 0) {
      return &(d.ents[i]);
    }
  }
  return 0;
}


int
get_stat(const char* path, struct stat* st)
{
  dirent* d = get_dirent(path);
  if(d) {
    pnode* p = d->node;
    memset(st, 0, sizeof(struct stat));
    st->st_uid  = getuid();
    st->st_mode = p->mode;
    st->st_size = p->size;
    return 0;
  }
  return -1;
}

char*
get_data(const char* path)
{
  dirent* d = get_dirent(path);
  if(d) {
    return d->name;
  }
  return 0;
}

int
put_data(const char* path, mode_t mode) {
  directory d = directory_from_path(path);
  if(d.pnum) {
    slist* s = directory_list(path);
    while(s->next != 0) {
      s = s->next;
    }
    directory_put_ent(d, s->data, mode);
    return 0;
  }
  return -1;
}

void*
get_page(const char* path)
{
  dirent* d = get_dirent(path);
  if(d) {
    return pages_get_page(d->pnum);
  }
  return 0;
} 

int
change_permissions(const char* path, mode_t mode) {
  dirent* d = get_dirent(path);
  if(d) {
    pnode* p = d->node;
    p->mode = mode;
    return 0;
  }
  return -1;
}

int
change_size(const char* path, off_t size) {
  dirent* d = get_dirent(path);
  if(d) {
    pnode* p = d->node;
    p->size = size;
    return 0;
  }
  return -1;
}

int
delete_data(const char* path) {
  return directory_delete(directory_from_path(path), get_dirent(path)->name);
}

char*
get_directory_item(const char* path, int i) {
  directory d = directory_from_path(path);
  if(d.ents[i].node && d.ents[i].node->refs != 0) {
    char* name = malloc(sizeof(char*));
    strcpy(name, path);
    strcat(name, d.ents[i].name);
    return name;
  }
  return 0;
}

