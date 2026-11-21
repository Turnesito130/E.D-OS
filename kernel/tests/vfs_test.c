#include <assert.h>
#include <stdio.h>
#include "vfs.h"
int main(void) {
    edos_ramfs_t ramfs;
    edos_vfs_t vfs;
    edos_vfs_dirent_t info;
    char out[16] = {0};
    ed_u64 cursor = 0;
    edos_ramfs_init(&ramfs);
    assert(!edos_vfs_mount_root(&vfs, 0));
    assert(edos_vfs_mount_root(&vfs, &ramfs));
    int fd = edos_vfs_open(&vfs, "/notes.txt", EDOS_VFS_CREATE | EDOS_VFS_READ | EDOS_VFS_WRITE);
    assert(fd == EDOS_VFS_FD_BASE);
    assert(edos_vfs_write(&vfs, fd, "hello", 5) == 5);
    assert(edos_vfs_seek(&vfs, fd, 0, EDOS_VFS_SEEK_SET));
    assert(edos_vfs_read(&vfs, fd, out, sizeof(out)) == 5);
    assert(out[0] == 'h' && out[4] == 'o');
    assert(edos_vfs_stat(&vfs, "/notes.txt", &info) && info.size == 5);
    assert(edos_vfs_seek(&vfs, fd, 1, EDOS_VFS_SEEK_END));
    assert(edos_vfs_write(&vfs, fd, "!", 1) == 1);
    assert(edos_vfs_stat(&vfs, "/notes.txt", &info) && info.size == 7);
    assert(edos_vfs_readdir(&vfs, &cursor, &info) && info.size == 7);
    assert(!edos_vfs_readdir(&vfs, &cursor, &info));
    assert(!edos_vfs_open(&vfs, "/../secret", EDOS_VFS_READ));
    assert(!edos_vfs_open(&vfs, "/dir/file", EDOS_VFS_READ));
    assert(!edos_vfs_open(&vfs, "relative", EDOS_VFS_READ));
    assert(edos_vfs_close(&vfs, fd));
    assert(!edos_vfs_read(&vfs, fd, out, sizeof(out)));
    int read_only = edos_vfs_open(&vfs, "/notes.txt", EDOS_VFS_READ);
    assert(read_only >= EDOS_VFS_FD_BASE);
    assert(!edos_vfs_write(&vfs, read_only, "x", 1));
    assert(edos_vfs_close(&vfs, read_only));
    int truncated = edos_vfs_open(&vfs, "/notes.txt", EDOS_VFS_WRITE | EDOS_VFS_TRUNC);
    assert(truncated >= EDOS_VFS_FD_BASE);
    assert(edos_vfs_stat(&vfs, "/notes.txt", &info) && info.size == 0);
    puts("VFS_TESTS_OK");
    return 0;
}
