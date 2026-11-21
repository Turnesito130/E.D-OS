#include "vfs.h"

#define EDOS_VFS_BACKEND_RAMFS 1U
#define EDOS_VFS_BACKEND_EDFS 2U

static int copy_path_name(const char *path, char *name) {
    if (!path || !name || path[0] != '/') return 0;
    ed_u64 i = 1;
    if (!path[i] || path[i] == '/' || path[i] == '.' || path[i] == '\\') return 0;
    while (path[i] && i <= EDOS_RAMFS_NAME_MAX) {
        if (path[i] == '/' || path[i] == '\\') return 0;
        name[i - 1U] = path[i];
        ++i;
    }
    if (path[i] || i == 1U) return 0;
    name[i - 1U] = 0;
    return 1;
}
static edos_ramfs_file_t *find_ramfs_entry(edos_vfs_t *vfs, const char *name) {
    if (!vfs || !vfs->mounted || vfs->backend != EDOS_VFS_BACKEND_RAMFS ||
        !vfs->root || !name) return (edos_ramfs_file_t *)0;
    for (unsigned int i = 0; i < EDOS_RAMFS_MAX_FILES; ++i) {
        edos_ramfs_file_t *file = &vfs->root->files[i];
        unsigned int j = 0;
        if (!file->used) continue;
        while (file->name[j] && name[j] && file->name[j] == name[j]) ++j;
        if (!file->name[j] && !name[j]) return file;
    }
    return (edos_ramfs_file_t *)0;
}
static int file_exists(edos_vfs_t *vfs, const char *name) {
    if (vfs->backend == EDOS_VFS_BACKEND_RAMFS)
        return find_ramfs_entry(vfs, name) != (edos_ramfs_file_t *)0;
    if (vfs->backend == EDOS_VFS_BACKEND_EDFS) {
        edos_edfs_dirent_t info;
        return edos_edfs_stat(vfs->edfs, name, &info);
    }
    return 0;
}
static ed_u64 file_size(edos_vfs_t *vfs, const char *name) {
    if (vfs->backend == EDOS_VFS_BACKEND_RAMFS) {
        edos_ramfs_file_t *file = find_ramfs_entry(vfs, name);
        return file ? file->size : 0;
    }
    if (vfs->backend == EDOS_VFS_BACKEND_EDFS) {
        edos_edfs_dirent_t info;
        return edos_edfs_stat(vfs->edfs, name, &info) ? info.size : 0;
    }
    return 0;
}
static int backend_create(edos_vfs_t *vfs, const char *name) {
    if (vfs->backend == EDOS_VFS_BACKEND_RAMFS) return edos_ramfs_create(vfs->root, name);
    if (vfs->backend == EDOS_VFS_BACKEND_EDFS) return edos_edfs_create(vfs->edfs, name);
    return 0;
}
static int backend_truncate(edos_vfs_t *vfs, const char *name) {
    if (vfs->backend == EDOS_VFS_BACKEND_RAMFS)
        return edos_ramfs_write(vfs->root, name, (const void *)0, 0);
    if (vfs->backend == EDOS_VFS_BACKEND_EDFS)
        return edos_edfs_write(vfs->edfs, name, (const void *)0, 0);
    return 0;
}
static ed_u64 backend_read_at(edos_vfs_t *vfs, const char *name, ed_u64 offset,
                              void *buffer, ed_u64 length) {
    if (vfs->backend == EDOS_VFS_BACKEND_RAMFS)
        return edos_ramfs_read_at(vfs->root, name, offset, buffer, length);
    if (vfs->backend == EDOS_VFS_BACKEND_EDFS)
        return edos_edfs_read_at(vfs->edfs, name, offset, buffer, length);
    return 0;
}
static int backend_write_at(edos_vfs_t *vfs, const char *name, ed_u64 offset,
                            const void *buffer, ed_u64 length) {
    if (vfs->backend == EDOS_VFS_BACKEND_RAMFS)
        return edos_ramfs_write_at(vfs->root, name, offset, buffer, length);
    if (vfs->backend == EDOS_VFS_BACKEND_EDFS)
        return edos_edfs_write_at(vfs->edfs, name, offset, buffer, length);
    return 0;
}
static ed_u64 backend_capacity(const edos_vfs_t *vfs) {
    return vfs->backend == EDOS_VFS_BACKEND_EDFS ? EDOS_EDFS_FILE_MAX :
           vfs->backend == EDOS_VFS_BACKEND_RAMFS ? EDOS_RAMFS_FILE_CAPACITY : 0;
}
static void clear_descriptors(edos_vfs_t *vfs) {
    for (unsigned int i = 0; i < EDOS_VFS_MAX_FDS; ++i) {
        vfs->descriptors[i].used = 0;
        vfs->descriptors[i].position = 0;
        vfs->descriptors[i].flags = 0;
        vfs->descriptors[i].name[0] = 0;
    }
}
static edos_vfs_fd_t *get_fd(edos_vfs_t *vfs, int fd) {
    if (!vfs || !vfs->mounted || fd < EDOS_VFS_FD_BASE ||
        fd >= EDOS_VFS_FD_BASE + (int)EDOS_VFS_MAX_FDS) return (edos_vfs_fd_t *)0;
    edos_vfs_fd_t *entry = &vfs->descriptors[(unsigned int)(fd - EDOS_VFS_FD_BASE)];
    return entry->used ? entry : (edos_vfs_fd_t *)0;
}
int edos_vfs_mount_root(edos_vfs_t *vfs, edos_ramfs_t *ramfs) {
    if (!vfs || !ramfs || !ramfs->initialized) return 0;
    vfs->root = ramfs;
    vfs->edfs = (edos_edfs_t *)0;
    vfs->backend = EDOS_VFS_BACKEND_RAMFS;
    vfs->mounted = 1;
    clear_descriptors(vfs);
    return 1;
}
int edos_vfs_mount_edfs(edos_vfs_t *vfs, edos_edfs_t *edfs) {
    if (!vfs || !edfs || !edfs->initialized) return 0;
    vfs->root = (edos_ramfs_t *)0;
    vfs->edfs = edfs;
    vfs->backend = EDOS_VFS_BACKEND_EDFS;
    vfs->mounted = 1;
    clear_descriptors(vfs);
    return 1;
}
int edos_vfs_open(edos_vfs_t *vfs, const char *path, unsigned int flags) {
    char name[EDOS_RAMFS_NAME_MAX + 1U];
    if (!vfs || !vfs->mounted || !copy_path_name(path, name) ||
        !(flags & (EDOS_VFS_READ | EDOS_VFS_WRITE)) ||
        ((flags & EDOS_VFS_TRUNC) && !(flags & EDOS_VFS_WRITE)) ||
        (flags & ~(EDOS_VFS_READ | EDOS_VFS_WRITE | EDOS_VFS_CREATE | EDOS_VFS_TRUNC)))
        return -1;
    unsigned int free_slot = EDOS_VFS_MAX_FDS;
    for (unsigned int i = 0; i < EDOS_VFS_MAX_FDS; ++i)
        if (!vfs->descriptors[i].used) { free_slot = i; break; }
    if (free_slot == EDOS_VFS_MAX_FDS) return -1;
    if (!file_exists(vfs, name)) {
        if (!(flags & EDOS_VFS_CREATE) || !backend_create(vfs, name)) return -1;
    }
    if ((flags & EDOS_VFS_TRUNC) && !backend_truncate(vfs, name)) return -1;
    edos_vfs_fd_t *entry = &vfs->descriptors[free_slot];
    unsigned int j = 0;
    while (name[j]) { entry->name[j] = name[j]; ++j; }
    entry->name[j] = 0;
    entry->position = 0;
    entry->flags = flags;
    entry->used = 1;
    return EDOS_VFS_FD_BASE + (int)free_slot;
}
int edos_vfs_close(edos_vfs_t *vfs, int fd) {
    edos_vfs_fd_t *entry = get_fd(vfs, fd);
    if (!entry) return 0;
    entry->used = 0;
    entry->position = 0;
    entry->flags = 0;
    entry->name[0] = 0;
    return 1;
}
ed_u64 edos_vfs_read(edos_vfs_t *vfs, int fd, void *buffer, ed_u64 length) {
    edos_vfs_fd_t *entry = get_fd(vfs, fd);
    if (!entry || !(entry->flags & EDOS_VFS_READ) || (!buffer && length)) return 0;
    ed_u64 count = backend_read_at(vfs, entry->name, entry->position, buffer, length);
    entry->position += count;
    return count;
}
ed_u64 edos_vfs_write(edos_vfs_t *vfs, int fd, const void *buffer, ed_u64 length) {
    edos_vfs_fd_t *entry = get_fd(vfs, fd);
    if (!entry || !(entry->flags & EDOS_VFS_WRITE) || (!buffer && length)) return 0;
    if (!backend_write_at(vfs, entry->name, entry->position, buffer, length)) return 0;
    entry->position += length;
    return length;
}
int edos_vfs_seek(edos_vfs_t *vfs, int fd, ed_u64 offset, unsigned int whence) {
    edos_vfs_fd_t *entry = get_fd(vfs, fd);
    if (!entry) return 0;
    ed_u64 base = 0, capacity = backend_capacity(vfs);
    if (whence == EDOS_VFS_SEEK_CUR) base = entry->position;
    else if (whence == EDOS_VFS_SEEK_END) base = file_size(vfs, entry->name);
    else if (whence != EDOS_VFS_SEEK_SET) return 0;
    if (offset > capacity || base > capacity - offset) return 0;
    entry->position = base + offset;
    return 1;
}
int edos_vfs_stat(edos_vfs_t *vfs, const char *path, edos_vfs_dirent_t *result) {
    char name[EDOS_RAMFS_NAME_MAX + 1U];
    if (!vfs || !vfs->mounted || !result || !copy_path_name(path, name)) return 0;
    if (vfs->backend == EDOS_VFS_BACKEND_RAMFS) {
        edos_ramfs_file_t *file = find_ramfs_entry(vfs, name);
        if (!file) return 0;
        unsigned int i = 0;
        while (file->name[i]) { result->name[i] = file->name[i]; ++i; }
        result->name[i] = 0;
        result->size = file->size;
        return 1;
    }
    if (vfs->backend == EDOS_VFS_BACKEND_EDFS) {
        edos_edfs_dirent_t info;
        if (!edos_edfs_stat(vfs->edfs, name, &info)) return 0;
        unsigned int i = 0;
        while (info.name[i]) { result->name[i] = info.name[i]; ++i; }
        result->name[i] = 0;
        result->size = info.size;
        return 1;
    }
    return 0;
}
int edos_vfs_readdir(edos_vfs_t *vfs, ed_u64 *cursor, edos_vfs_dirent_t *result) {
    if (!vfs || !vfs->mounted || !cursor || !result) return 0;
    if (vfs->backend == EDOS_VFS_BACKEND_RAMFS) {
        while (*cursor < EDOS_RAMFS_MAX_FILES) {
            edos_ramfs_file_t *file = &vfs->root->files[(unsigned int)(*cursor)++];
            if (!file->used) continue;
            unsigned int i = 0;
            while (file->name[i]) { result->name[i] = file->name[i]; ++i; }
            result->name[i] = 0;
            result->size = file->size;
            return 1;
        }
        return 0;
    }
    if (vfs->backend == EDOS_VFS_BACKEND_EDFS) {
        edos_edfs_dirent_t info;
        if (!edos_edfs_readdir(vfs->edfs, cursor, &info)) return 0;
        unsigned int i = 0;
        while (info.name[i]) { result->name[i] = info.name[i]; ++i; }
        result->name[i] = 0;
        result->size = info.size;
        return 1;
    }
    return 0;
}
