#include <assert.h>
#include <stdio.h>
#include "block.h"
int main(void) {
    unsigned char storage[4096] = {0};
    unsigned char input[1024], output[1024];
    edos_block_device_t device;
    edos_ramdisk_t disk;
    for (unsigned int i = 0; i < sizeof(input); ++i) input[i] = (unsigned char)(i & 0xffU);
    assert(edos_ramdisk_init(&device, &disk, storage, sizeof(storage), 512U));
    assert(device.initialized && device.sector_count == 8 && device.sector_size == 512U);
    assert(edos_block_write(&device, 2, 2, input));
    assert(edos_block_read(&device, 2, 2, output));
    for (unsigned int i = 0; i < sizeof(input); ++i) assert(input[i] == output[i]);
    assert(!edos_block_read(&device, 8, 1, output));
    assert(!edos_block_read(&device, 7, 2, output));
    assert(!edos_block_write(&device, 0, 0, input));
    assert(!edos_block_write(&device, 0, 1, 0));
    edos_block_device_t readonly_device;
    assert(edos_block_device_init(&readonly_device, &disk, 8, 512U, device.read, 0, 1));
    assert(edos_block_read(&readonly_device, 2, 2, output));
    assert(!edos_block_write(&readonly_device, 2, 2, input));
    assert(!edos_block_device_init(&device, &disk, 8, 300U, device.read, device.write, 0));
    puts("BLOCK_DEVICE_TESTS_OK");
    return 0;
}
