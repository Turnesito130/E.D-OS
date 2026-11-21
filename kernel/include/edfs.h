#ifndef EDOS_EDFS_H
#define EDOS_EDFS_H
#include "block.h"
#define EDOS_EDFS_MAX_FILES 16U
#define EDOS_EDFS_NAME_MAX 25U
#define EDOS_EDFS_FILE_MAX 512U
typedef struct { edos_block_device_t *device; int initialized; } edos_edfs_t;
int edos_edfs_format(edos_block_device_t *device);
int edos_edfs_mount(edos_edfs_t *fs, edos_block_device_t *device);
int edos_edfs_create(edos_edfs_t *fs, const char *name);
int edos_edfs_write(edos_edfs_t *fs, const char *name, const void *data, ed_u64 length);
ed_u64 edos_edfs_read(edos_edfs_t *fs, const char *name, void *buffer, ed_u64 capacity);
int edos_edfs_delete(edos_edfs_t *fs, const char *name);
#endif
