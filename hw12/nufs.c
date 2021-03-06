#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <bsd/string.h>
#include <assert.h>
#include <stdlib.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"


// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char* path, int mask)
{
  printf("access(%s, %04o)\n", path, mask);
  struct stat* s = malloc(sizeof(struct stat*));
  if(get_stat(path, s) != -1) {
    //if(s->st_mode implement mode check
    return 0;
  }
  return -ENOENT;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
  printf("getattr(%s)\n", path);
  int rv = get_stat(path, st);
  if (rv == -1) {
    return -ENOENT;
  }
  else {
    return 0;
  }
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
             off_t offset, struct fuse_file_info *fi)
{
  struct stat st;
  printf("readdir(%s)\n", path);
  get_stat(path, &st);
  // filler is a callback that adds one item to the result
  // it will return non-zero when the buffer is full
  filler(buf, ".", &st, 0);
  filler(buf, "..", &st, 0);
  
  char* item;
  int i = 1;
  while(item = get_directory_item(path, i)) {
    if(get_stat(item, &st) != -1) {
      filler(buf, get_data(item), &st, 0);
    }
    i++;
  }

  return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
  printf("mknod(%s, %04o)\n", path, mode);
  struct stat st;
  get_stat("/", &st);
  if(put_data(path, mode) != -1) {
    return 0;
  }
  return -ENOENT;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
  printf("mkdir(%s)\n", path);
  return -1;
}

int
nufs_unlink(const char *path)
{
  
  if(delete_data(path) == 0) {
    return 0;
  } 
  return -ENOENT;
}

int
nufs_rmdir(const char *path)
{
  printf("rmdir(%s)\n", path);
  return -1;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
  char* s = get_data(from);
  if(s) {
    strcpy(s, to);
    return 0;
  }
  return -ENOENT;
}

int
nufs_chmod(const char *path, mode_t mode)
{
  return change_permissions(path, mode);
}

int
nufs_truncate(const char *path, off_t size)
{
  printf("truncate(%s, %ld bytes)\n", path, size);
  return change_size(path, size);
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
  printf("open(%s)\n", path);
  struct stat* s = malloc(sizeof(struct stat*));
  if(get_stat(path, s) != -1) {
    return 0;
  }
  return -1;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  printf("read(%s, %ld bytes, @%ld)\n", path, size, offset);
  void* data = get_page(path);

  int len = strlen(data) + 1;
  if (size < len) {
    len = size;
  }

  memcpy(buf, data + offset, len);
  return len;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
  printf("write(%s, %ld bytes, @%ld)\n", path, size, offset);
  void* data = get_page(path);

  int len = strlen(data) + 1;
  if (size < len) {
    len = size;
  }

  memcpy(data + offset, buf, len);
  return len;
}

void
nufs_init_ops(struct fuse_operations* ops)
{
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access   = nufs_access;
  ops->getattr  = nufs_getattr;
  ops->readdir  = nufs_readdir;
  ops->mknod    = nufs_mknod;
  ops->mkdir    = nufs_mkdir;
  ops->unlink   = nufs_unlink;
  ops->rmdir    = nufs_rmdir;
  ops->rename   = nufs_rename;
  ops->chmod    = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open	  = nufs_open;
  ops->read     = nufs_read;
  ops->write    = nufs_write;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
  assert(argc > 2);
  storage_init(argv[--argc]);
  nufs_init_ops(&nufs_ops);
  return fuse_main(argc, argv, &nufs_ops, NULL);
}

