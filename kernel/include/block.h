#ifndef EDOS_BLOCK_H
#define EDOS_BLOCK_H
#include "memory.h"
#define EDOS_BLOCK_DEFAULT_SECTOR_SIZE 512U
typedef int (*edos_block_read_fn)(void *context, ed_u64 lba, ed_u64 count, void *buffer);
typedef int (*edos_block_write_fn)(void *context, ed_u64 lba, ed_u64 count, const void *buffer);
typedef struct {
    void *context;
    ed_u64 sector_count;
    unsigned int sector_size;
    edos_block_read_fn read;
    edos_block_write_fn write;
    int read_only;
    int initialized;
} edos_block_device_t;
typedef struct {
    unsigned char *storage;
    ed_u64 sector_count;
    unsigned int sector_size;
} edos_ramdisk_t;
int edos_block_device_init(edos_block_device_t *device, void *context,
                           ed_u64 sector_count, unsigned int sector_size,
                           edos_block_read_fn read_fn, edos_block_write_fn write_fn,
                           int read_only);
int edos_block_read(edos_block_device_t *device, ed_u64 lba, ed_u64 count, void *buffer);
int edos_block_write(edos_block_device_t *device, ed_u64 lba, ed_u64 count, const void *buffer);
int edos_ramdisk_init(edos_block_device_t *device, edos_ramdisk_t *ramdisk,
                      void *storage, ed_u64 storage_bytes, unsigned int sector_size);
#endif
