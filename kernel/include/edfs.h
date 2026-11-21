#ifndef EDOS_EDFS_H
#define EDOS_EDFS_H
#include "block.h"
#define EDOS_EDFS_MAX_FILES 16U
#define EDOS_EDFS_NAME_MAX 25U
#define EDOS_EDFS_FILE_MAX 512U
typedef struct { edos_block_device_t *device; int initialized; } edos_edfs_t;
typedef struct { char name[EDOS_EDFS_NAME_MAX+1U]; ed_u64 size; } edos_edfs_dirent_t;
int edos_edfs_format(edos_block_device_t *device);
int edos_edfs_mount(edos_edfs_t *fs, edos_block_device_t *device);
int edos_edfs_create(edos_edfs_t *fs, const char *name);
int edos_edfs_write(edos_edfs_t *fs, const char *name, const void *data, ed_u64 length);
ed_u64 edos_edfs_read(edos_edfs_t *fs, const char *name, void *buffer, ed_u64 capacity);
ed_u64 edos_edfs_read_at(edos_edfs_t *fs, const char *name, ed_u64 offset, void *buffer, ed_u64 capacity);
int edos_edfs_write_at(edos_edfs_t *fs, const char *name, ed_u64 offset, const void *data, ed_u64 length);
int edos_edfs_stat(edos_edfs_t *fs, const char *name, edos_edfs_dirent_t *result);
int edos_edfs_readdir(edos_edfs_t *fs, ed_u64 *cursor, edos_edfs_dirent_t *result);
int edos_edfs_delete(edos_edfs_t *fs, const char *name);
#endif
