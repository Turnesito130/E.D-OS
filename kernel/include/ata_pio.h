#ifndef EDOS_ATA_PIO_H
#define EDOS_ATA_PIO_H
#include "block.h"
typedef struct {
    ed_u64 sector_count;
    unsigned int slave;
    int present;
} edos_ata_pio_t;
/* Detects primary-channel ATA PIO master/slave and exposes it as a block device. */
int edos_ata_pio_init(edos_block_device_t *device, edos_ata_pio_t *ata,
                      unsigned int slave);
#endif
