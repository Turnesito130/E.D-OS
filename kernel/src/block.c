#include "block.h"
static int ramdisk_read(void *context, ed_u64 lba, ed_u64 count, void *buffer) {
    edos_ramdisk_t *disk = (edos_ramdisk_t *)context;
    if (!disk || !disk->storage || !buffer || lba > disk->sector_count ||
        count > disk->sector_count - lba) return 0;
    ed_u64 offset = lba * disk->sector_size;
    ed_u64 bytes = count * disk->sector_size;
    unsigned char *dst = (unsigned char *)buffer;
    for (ed_u64 i = 0; i < bytes; ++i) dst[i] = disk->storage[offset + i];
    return 1;
}
static int ramdisk_write(void *context, ed_u64 lba, ed_u64 count, const void *buffer) {
    edos_ramdisk_t *disk = (edos_ramdisk_t *)context;
    if (!disk || !disk->storage || !buffer || lba > disk->sector_count ||
        count > disk->sector_count - lba) return 0;
    ed_u64 offset = lba * disk->sector_size;
    ed_u64 bytes = count * disk->sector_size;
    const unsigned char *src = (const unsigned char *)buffer;
    for (ed_u64 i = 0; i < bytes; ++i) disk->storage[offset + i] = src[i];
    return 1;
}
int edos_block_device_init(edos_block_device_t *device, void *context,
                           ed_u64 sector_count, unsigned int sector_size,
                           edos_block_read_fn read_fn, edos_block_write_fn write_fn,
                           int read_only) {
    if (!device || !context || !sector_count || sector_size < 128U ||
        sector_size > 4096U || (sector_size & (sector_size - 1U)) ||
        !read_fn || (!read_only && !write_fn) || (read_only != 0 && read_only != 1))
        return 0;
    device->context = context;
    device->sector_count = sector_count;
    device->sector_size = sector_size;
    device->read = read_fn;
    device->write = write_fn;
    device->read_only = read_only;
    device->initialized = 1;
    return 1;
}
int edos_block_read(edos_block_device_t *device, ed_u64 lba, ed_u64 count, void *buffer) {
    if (!device || !device->initialized || !buffer || !count ||
        lba >= device->sector_count || count > device->sector_count - lba) return 0;
    return device->read(device->context, lba, count, buffer);
}
int edos_block_write(edos_block_device_t *device, ed_u64 lba, ed_u64 count, const void *buffer) {
    if (!device || !device->initialized || device->read_only || !buffer || !count ||
        lba >= device->sector_count || count > device->sector_count - lba) return 0;
    return device->write(device->context, lba, count, buffer);
}
int edos_ramdisk_init(edos_block_device_t *device, edos_ramdisk_t *ramdisk,
                      void *storage, ed_u64 storage_bytes, unsigned int sector_size) {
    if (!device || !ramdisk || !storage || !sector_size ||
        storage_bytes < sector_size || storage_bytes % sector_size) return 0;
    ramdisk->storage = (unsigned char *)storage;
    ramdisk->sector_size = sector_size;
    ramdisk->sector_count = storage_bytes / sector_size;
    return edos_block_device_init(device, ramdisk, ramdisk->sector_count,
                                  sector_size, ramdisk_read, ramdisk_write, 0);
}
