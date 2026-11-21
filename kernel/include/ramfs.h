#ifndef EDOS_RAMFS_H
#define EDOS_RAMFS_H
#include "memory.h"
#define EDOS_RAMFS_MAX_FILES 16U
#define EDOS_RAMFS_NAME_MAX 31U
#define EDOS_RAMFS_FILE_CAPACITY 256U
typedef struct { char name[EDOS_RAMFS_NAME_MAX+1U]; unsigned char data[EDOS_RAMFS_FILE_CAPACITY]; ed_u64 size; int used; } edos_ramfs_file_t;
typedef struct { edos_ramfs_file_t files[EDOS_RAMFS_MAX_FILES]; int initialized; } edos_ramfs_t;
void edos_ramfs_init(edos_ramfs_t *fs);
int edos_ramfs_create(edos_ramfs_t *fs,const char *name);
int edos_ramfs_write(edos_ramfs_t *fs,const char *name,const void *data,ed_u64 length);
ed_u64 edos_ramfs_read(edos_ramfs_t *fs,const char *name,void *buffer,ed_u64 capacity);
ed_u64 edos_ramfs_read_at(edos_ramfs_t *fs,const char *name,ed_u64 offset,void *buffer,ed_u64 capacity);
int edos_ramfs_write_at(edos_ramfs_t *fs,const char *name,ed_u64 offset,const void *data,ed_u64 length);
int edos_ramfs_delete(edos_ramfs_t *fs,const char *name);
#endif
