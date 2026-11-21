#ifndef EDOS_VFS_H
#define EDOS_VFS_H
#include "ramfs.h"
#include "edfs.h"
#define EDOS_VFS_MAX_FDS 16U
#define EDOS_VFS_PATH_MAX 64U
#define EDOS_VFS_FD_BASE 3
#define EDOS_VFS_READ  0x01U
#define EDOS_VFS_WRITE 0x02U
#define EDOS_VFS_CREATE 0x04U
#define EDOS_VFS_TRUNC 0x08U
#define EDOS_VFS_SEEK_SET 0U
#define EDOS_VFS_SEEK_CUR 1U
#define EDOS_VFS_SEEK_END 2U
typedef struct { char name[EDOS_RAMFS_NAME_MAX+1U]; ed_u64 size; } edos_vfs_dirent_t;
typedef struct { char name[EDOS_RAMFS_NAME_MAX+1U]; ed_u64 position; unsigned int flags; int used; } edos_vfs_fd_t;
typedef struct { edos_ramfs_t *root; edos_edfs_t *edfs; edos_vfs_fd_t descriptors[EDOS_VFS_MAX_FDS]; unsigned int backend; int mounted; } edos_vfs_t;
int edos_vfs_mount_root(edos_vfs_t *vfs, edos_ramfs_t *ramfs);
int edos_vfs_mount_edfs(edos_vfs_t *vfs, edos_edfs_t *edfs);
int edos_vfs_open(edos_vfs_t *vfs, const char *path, unsigned int flags);
int edos_vfs_close(edos_vfs_t *vfs, int fd);
ed_u64 edos_vfs_read(edos_vfs_t *vfs, int fd, void *buffer, ed_u64 length);
ed_u64 edos_vfs_write(edos_vfs_t *vfs, int fd, const void *buffer, ed_u64 length);
int edos_vfs_seek(edos_vfs_t *vfs, int fd, ed_u64 offset, unsigned int whence);
int edos_vfs_stat(edos_vfs_t *vfs, const char *path, edos_vfs_dirent_t *result);
int edos_vfs_readdir(edos_vfs_t *vfs, ed_u64 *cursor, edos_vfs_dirent_t *result);
#endif
