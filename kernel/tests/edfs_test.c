#include <assert.h>
#include <stdio.h>
#include "edfs.h"
int main(void) {
    unsigned char storage[18U * 512U] = {0};
    char output[32] = {0};
    edos_block_device_t device;
    edos_ramdisk_t disk;
    edos_edfs_t fs, remounted;
    assert(edos_ramdisk_init(&device, &disk, storage, sizeof(storage), 512U));
    assert(edos_edfs_format(&device));
    assert(edos_edfs_mount(&fs, &device));
    assert(edos_edfs_create(&fs, "kernel.log"));
    assert(!edos_edfs_create(&fs, "kernel.log"));
    assert(!edos_edfs_create(&fs, "../escape"));
    assert(edos_edfs_write(&fs, "kernel.log", "E.D-OS persistent", 17));
    assert(edos_edfs_mount(&remounted, &device));
    assert(edos_edfs_read(&remounted, "kernel.log", output, sizeof(output)) == 17);
    assert(output[0] == 'E' && output[16] == 't');
    assert(!edos_edfs_write(&remounted, "kernel.log", output, 513));
    assert(edos_edfs_delete(&remounted, "kernel.log"));
    assert(edos_edfs_read(&remounted, "kernel.log", output, sizeof(output)) == 0);
    storage[0] = 0;
    assert(!edos_edfs_mount(&remounted, &device));
    puts("EDFS_TESTS_OK");
    return 0;
}
