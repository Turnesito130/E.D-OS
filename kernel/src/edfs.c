#include "edfs.h"
#define EDFS_DIRECTORY_SECTOR 1ULL
#define EDFS_DATA_START 2ULL
#define EDFS_ENTRY_SIZE 32U
#define EDFS_DIRECTORY_BYTES 512U
static void zero_bytes(unsigned char *p, ed_u64 n) {
    for (ed_u64 i = 0; i < n; ++i) p[i] = 0;
}
static void copy_bytes(unsigned char *dst, const unsigned char *src, ed_u64 n) {
    for (ed_u64 i = 0; i < n; ++i) dst[i] = src[i];
}
static int valid_name(const char *name) {
    if (!name || !name[0]) return 0;
    ed_u64 n = 0;
    while (name[n] && n <= EDOS_EDFS_NAME_MAX) {
        if (name[n] == '/' || name[n] == '\\') return 0;
        ++n;
    }
    return n > 0 && n <= EDOS_EDFS_NAME_MAX;
}
static unsigned int read_u32(const unsigned char *p) {
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8) | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}
static void write_u32(unsigned char *p, unsigned int value) {
    p[0]=(unsigned char)value; p[1]=(unsigned char)(value>>8);
    p[2]=(unsigned char)(value>>16); p[3]=(unsigned char)(value>>24);
}
static unsigned short read_u16(const unsigned char *p) {
    return (unsigned short)((unsigned short)p[0] | ((unsigned short)p[1] << 8));
}
static void write_u16(unsigned char *p, unsigned short value) {
    p[0]=(unsigned char)value; p[1]=(unsigned char)(value>>8);
}
static int read_directory(edos_edfs_t *fs, unsigned char *directory) {
    return fs && fs->initialized && fs->device &&
           edos_block_read(fs->device, EDFS_DIRECTORY_SECTOR, 1, directory);
}
static int find_entry(const unsigned char *directory, const char *name) {
    for (unsigned int i = 0; i < EDOS_EDFS_MAX_FILES; ++i) {
        const unsigned char *entry = directory + i * EDFS_ENTRY_SIZE;
        if (entry[0] != 1U) continue;
        unsigned int j = 0;
        while (j <= EDOS_EDFS_NAME_MAX && entry[1U+j] && name[j] &&
               entry[1U+j] == (unsigned char)name[j]) ++j;
        if (j <= EDOS_EDFS_NAME_MAX && !entry[1U+j] && !name[j]) return (int)i;
    }
    return -1;
}
int edos_edfs_format(edos_block_device_t *device) {
    if (!device || !device->initialized || device->read_only ||
        device->sector_size != EDOS_BLOCK_DEFAULT_SECTOR_SIZE ||
        device->sector_count < EDFS_DATA_START + EDOS_EDFS_MAX_FILES ||
        device->sector_count > 0xffffffffULL) return 0;
    unsigned char sector[EDOS_BLOCK_DEFAULT_SECTOR_SIZE];
    zero_bytes(sector, sizeof(sector));
    const unsigned char magic[8] = {'E','D','F','S','v','0','0','1'};
    copy_bytes(sector, magic, 8);
    write_u32(sector + 8, 1U);
    write_u32(sector + 12, (unsigned int)device->sector_count);
    write_u32(sector + 16, EDOS_EDFS_MAX_FILES);
    if (!edos_block_write(device, 0, 1, sector)) return 0;
    zero_bytes(sector, sizeof(sector));
    return edos_block_write(device, EDFS_DIRECTORY_SECTOR, 1, sector);
}
int edos_edfs_mount(edos_edfs_t *fs, edos_block_device_t *device) {
    if (!fs || !device || !device->initialized ||
        device->sector_size != EDOS_BLOCK_DEFAULT_SECTOR_SIZE) return 0;
    fs->initialized = 0;
    unsigned char header[EDOS_BLOCK_DEFAULT_SECTOR_SIZE];
    if (!edos_block_read(device, 0, 1, header)) return 0;
    const unsigned char magic[8] = {'E','D','F','S','v','0','0','1'};
    for (unsigned int i = 0; i < 8; ++i) if (header[i] != magic[i]) return 0;
    if (read_u32(header+8) != 1U || read_u32(header+12) > device->sector_count ||
        read_u32(header+12) < EDFS_DATA_START + EDOS_EDFS_MAX_FILES ||
        read_u32(header+16) != EDOS_EDFS_MAX_FILES) return 0;
    unsigned char directory[EDFS_DIRECTORY_BYTES];
    if (!edos_block_read(device, EDFS_DIRECTORY_SECTOR, 1, directory)) return 0;
    for (unsigned int i = 0; i < EDOS_EDFS_MAX_FILES; ++i) {
        unsigned char *entry = directory + i * EDFS_ENTRY_SIZE;
        if (entry[0] > 1U || (entry[0] && (read_u16(entry+27) > EDOS_EDFS_FILE_MAX || !entry[1]))) return 0;
        if (!entry[0]) continue;
        unsigned int n = 0;
        while (n <= EDOS_EDFS_NAME_MAX && entry[1+n]) ++n;
        if (!n || n > EDOS_EDFS_NAME_MAX) return 0;
        for (unsigned int j = i + 1U; j < EDOS_EDFS_MAX_FILES; ++j) {
            unsigned char *other = directory + j * EDFS_ENTRY_SIZE;
            if (other[0] != 1U) continue;
            unsigned int k = 0;
            while (k <= EDOS_EDFS_NAME_MAX && entry[1+k] && other[1+k] &&
                   entry[1+k] == other[1+k]) ++k;
            if (!entry[1+k] && !other[1+k]) return 0;
        }
    }
    fs->device = device;
    fs->initialized = 1;
    return 1;
}
int edos_edfs_create(edos_edfs_t *fs, const char *name) {
    if (!fs || !fs->initialized || !valid_name(name)) return 0;
    unsigned char directory[EDFS_DIRECTORY_BYTES];
    if (!read_directory(fs, directory) || find_entry(directory, name) >= 0) return 0;
    for (unsigned int i = 0; i < EDOS_EDFS_MAX_FILES; ++i) {
        unsigned char *entry = directory + i * EDFS_ENTRY_SIZE;
        if (entry[0]) continue;
        zero_bytes(entry, EDFS_ENTRY_SIZE);
        entry[0] = 1U;
        unsigned int j = 0;
        while (name[j]) { entry[1U+j] = (unsigned char)name[j]; ++j; }
        return edos_block_write(fs->device, EDFS_DIRECTORY_SECTOR, 1, directory);
    }
    return 0;
}
int edos_edfs_write(edos_edfs_t *fs, const char *name, const void *data, ed_u64 length) {
    if (!fs || !fs->initialized || !valid_name(name) || (!data && length) ||
        length > EDOS_EDFS_FILE_MAX) return 0;
    unsigned char directory[EDFS_DIRECTORY_BYTES];
    if (!read_directory(fs, directory)) return 0;
    int slot = find_entry(directory, name);
    if (slot < 0) return 0;
    unsigned char sector[EDOS_BLOCK_DEFAULT_SECTOR_SIZE];
    zero_bytes(sector, sizeof(sector));
    if (length) copy_bytes(sector, (const unsigned char *)data, length);
    if (!edos_block_write(fs->device, EDFS_DATA_START + (ed_u64)slot, 1, sector)) return 0;
    write_u16(directory + (unsigned int)slot * EDFS_ENTRY_SIZE + 27U, (unsigned short)length);
    return edos_block_write(fs->device, EDFS_DIRECTORY_SECTOR, 1, directory);
}
ed_u64 edos_edfs_read(edos_edfs_t *fs, const char *name, void *buffer, ed_u64 capacity) {
    if (!fs || !fs->initialized || !valid_name(name) || (!buffer && capacity)) return 0;
    unsigned char directory[EDFS_DIRECTORY_BYTES];
    if (!read_directory(fs, directory)) return 0;
    int slot = find_entry(directory, name);
    if (slot < 0) return 0;
    unsigned char sector[EDOS_BLOCK_DEFAULT_SECTOR_SIZE];
    if (!edos_block_read(fs->device, EDFS_DATA_START + (ed_u64)slot, 1, sector)) return 0;
    ed_u64 size = read_u16(directory + (unsigned int)slot * EDFS_ENTRY_SIZE + 27U);
    ed_u64 count = size < capacity ? size : capacity;
    if (count) copy_bytes((unsigned char *)buffer, sector, count);
    return count;
}
int edos_edfs_delete(edos_edfs_t *fs, const char *name) {
    if (!fs || !fs->initialized || !valid_name(name)) return 0;
    unsigned char directory[EDFS_DIRECTORY_BYTES];
    if (!read_directory(fs, directory)) return 0;
    int slot = find_entry(directory, name);
    if (slot < 0) return 0;
    zero_bytes(directory + (unsigned int)slot * EDFS_ENTRY_SIZE, EDFS_ENTRY_SIZE);
    return edos_block_write(fs->device, EDFS_DIRECTORY_SECTOR, 1, directory);
}
