#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

void storage_init(const char* path);
int         get_stat(const char* path, struct stat* st);
char* get_data(const char* path);
void* get_page(const char* path);
int put_data(const char* path, mode_t mode);
int change_permissions(const char* path, mode_t mode);
int change_size(const char* path, off_t size);
int delete_data(const char* path);
char* get_directory_item(const char* path, int i);

#endif
