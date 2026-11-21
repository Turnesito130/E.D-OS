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
    assert(edos_vfs_open(&vfs, "/../secret", EDOS_VFS_READ) == -1);
    assert(edos_vfs_open(&vfs, "/dir/file", EDOS_VFS_READ) == -1);
    assert(edos_vfs_open(&vfs, "relative", EDOS_VFS_READ) == -1);
    assert(edos_vfs_close(&vfs, fd));
    assert(!edos_vfs_read(&vfs, fd, out, sizeof(out)));
    int read_only = edos_vfs_open(&vfs, "/notes.txt", EDOS_VFS_READ);
    assert(read_only >= EDOS_VFS_FD_BASE);
    assert(!edos_vfs_write(&vfs, read_only, "x", 1));
    assert(edos_vfs_close(&vfs, read_only));
    assert(edos_vfs_open(&vfs, "/notes.txt", EDOS_VFS_READ | EDOS_VFS_TRUNC) == -1);
    int truncated = edos_vfs_open(&vfs, "/notes.txt", EDOS_VFS_WRITE | EDOS_VFS_TRUNC);
    assert(truncated >= EDOS_VFS_FD_BASE);
    assert(edos_vfs_stat(&vfs, "/notes.txt", &info) && info.size == 0);
    assert(edos_vfs_close(&vfs, truncated));
    int open_fds[EDOS_VFS_MAX_FDS];
    for (unsigned int i = 0; i < EDOS_VFS_MAX_FDS; ++i) {
        open_fds[i] = edos_vfs_open(&vfs, "/notes.txt", EDOS_VFS_READ);
        assert(open_fds[i] >= EDOS_VFS_FD_BASE);
    }
    assert(edos_vfs_open(&vfs, "/notes.txt", EDOS_VFS_READ) == -1);
    for (unsigned int i = 0; i < EDOS_VFS_MAX_FDS; ++i) assert(edos_vfs_close(&vfs, open_fds[i]));

    unsigned char disk_storage[18U * 512U] = {0};
    edos_block_device_t block_device;
    edos_ramdisk_t ramdisk;
    edos_edfs_t edfs, edfs_remounted;
    edos_vfs_t persistent_vfs;
    assert(edos_ramdisk_init(&block_device, &ramdisk, disk_storage, sizeof(disk_storage), 512U));
    assert(edos_edfs_format(&block_device));
    assert(edos_edfs_mount(&edfs, &block_device));
    assert(edos_vfs_mount_edfs(&persistent_vfs, &edfs));
    int persistent_fd = edos_vfs_open(&persistent_vfs, "/persist.log", EDOS_VFS_CREATE | EDOS_VFS_READ | EDOS_VFS_WRITE);
    assert(persistent_fd >= EDOS_VFS_FD_BASE);
    assert(edos_vfs_write(&persistent_vfs, persistent_fd, "persist", 7) == 7);
    assert(edos_vfs_close(&persistent_vfs, persistent_fd));
    assert(edos_edfs_mount(&edfs_remounted, &block_device));
    assert(edos_vfs_mount_edfs(&persistent_vfs, &edfs_remounted));
    persistent_fd = edos_vfs_open(&persistent_vfs, "/persist.log", EDOS_VFS_READ);
    assert(persistent_fd >= EDOS_VFS_FD_BASE);
    for (unsigned int i = 0; i < sizeof(out); ++i) out[i] = 0;
    assert(edos_vfs_read(&persistent_vfs, persistent_fd, out, sizeof(out)) == 7);
    assert(out[0] == 'p' && out[6] == 't');
    assert(edos_vfs_stat(&persistent_vfs, "/persist.log", &info) && info.size == 7);
    cursor = 0;
    assert(edos_vfs_readdir(&persistent_vfs, &cursor, &info) && info.size == 7);
    assert(edos_vfs_close(&persistent_vfs, persistent_fd));
    puts("VFS_TESTS_OK");
    return 0;
}
