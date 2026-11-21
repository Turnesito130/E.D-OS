#include "ata_pio.h"
#define ATA_DATA 0x1f0U
#define ATA_SECTOR_COUNT 0x1f2U
#define ATA_LBA_LOW 0x1f3U
#define ATA_LBA_MID 0x1f4U
#define ATA_LBA_HIGH 0x1f5U
#define ATA_DRIVE 0x1f6U
#define ATA_STATUS 0x1f7U
#define ATA_COMMAND 0x1f7U
#define ATA_ALT_STATUS 0x3f6U
#define ATA_CONTROL 0x3f6U
#define ATA_STATUS_ERR 0x01U
#define ATA_STATUS_DRQ 0x08U
#define ATA_STATUS_BSY 0x80U
#define ATA_POLL_LIMIT 1000000U
#define ATA_LBA28_LIMIT 0x10000000ULL

static unsigned char inb(unsigned short port) {
    unsigned char value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static unsigned short inw(unsigned short port) {
    unsigned short value;
    __asm__ volatile ("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
static void outb(unsigned short port, unsigned char value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static void outw(unsigned short port, unsigned short value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}
static void ata_delay_400ns(void) {
    (void)inb(ATA_ALT_STATUS); (void)inb(ATA_ALT_STATUS);
    (void)inb(ATA_ALT_STATUS); (void)inb(ATA_ALT_STATUS);
}
static int ata_wait(unsigned int require_drq) {
    for (unsigned int i = 0; i < ATA_POLL_LIMIT; ++i) {
        unsigned char status = inb(ATA_STATUS);
        if (status == 0U || status == 0xffU) return 0;
        if (status & ATA_STATUS_BSY) continue;
        if (status & ATA_STATUS_ERR) return 0;
        if (require_drq && !(status & ATA_STATUS_DRQ)) continue;
        return 1;
    }
    return 0;
}
static void ata_select(const edos_ata_pio_t *ata, ed_u64 lba) {
    outb(ATA_DRIVE, (unsigned char)(0xe0U | (ata->slave << 4) | ((lba >> 24) & 0x0fU)));
    ata_delay_400ns();
}
static int ata_read_sector(edos_ata_pio_t *ata, ed_u64 lba, unsigned char *buffer) {
    if (!ata || !ata->present || !buffer || lba >= ata->sector_count || lba >= ATA_LBA28_LIMIT)
        return 0;
    ata_select(ata, lba);
    outb(ATA_SECTOR_COUNT, 1U);
    outb(ATA_LBA_LOW, (unsigned char)lba);
    outb(ATA_LBA_MID, (unsigned char)(lba >> 8));
    outb(ATA_LBA_HIGH, (unsigned char)(lba >> 16));
    outb(ATA_COMMAND, 0x20U);
    if (!ata_wait(1U)) return 0;
    unsigned short *words = (unsigned short *)buffer;
    for (unsigned int i = 0; i < 256U; ++i) words[i] = inw(ATA_DATA);
    return ata_wait(0U);
}
static int ata_write_sector(edos_ata_pio_t *ata, ed_u64 lba, const unsigned char *buffer) {
    if (!ata || !ata->present || !buffer || lba >= ata->sector_count || lba >= ATA_LBA28_LIMIT)
        return 0;
    ata_select(ata, lba);
    outb(ATA_SECTOR_COUNT, 1U);
    outb(ATA_LBA_LOW, (unsigned char)lba);
    outb(ATA_LBA_MID, (unsigned char)(lba >> 8));
    outb(ATA_LBA_HIGH, (unsigned char)(lba >> 16));
    outb(ATA_COMMAND, 0x30U);
    if (!ata_wait(1U)) return 0;
    const unsigned short *words = (const unsigned short *)buffer;
    for (unsigned int i = 0; i < 256U; ++i) outw(ATA_DATA, words[i]);
    if (!ata_wait(0U)) return 0;
    outb(ATA_COMMAND, 0xe7U);
    return ata_wait(0U);
}
static int ata_block_read(void *context, ed_u64 lba, ed_u64 count, void *buffer) {
    edos_ata_pio_t *ata = (edos_ata_pio_t *)context;
    if (!ata || !buffer || !count || lba > ata->sector_count ||
        count > ata->sector_count - lba || lba >= ATA_LBA28_LIMIT ||
        count > ATA_LBA28_LIMIT - lba) return 0;
    unsigned char *bytes = (unsigned char *)buffer;
    for (ed_u64 i = 0; i < count; ++i)
        if (!ata_read_sector(ata, lba + i, bytes + i * 512ULL)) return 0;
    return 1;
}
static int ata_block_write(void *context, ed_u64 lba, ed_u64 count, const void *buffer) {
    edos_ata_pio_t *ata = (edos_ata_pio_t *)context;
    if (!ata || !buffer || !count || lba > ata->sector_count ||
        count > ata->sector_count - lba || lba >= ATA_LBA28_LIMIT ||
        count > ATA_LBA28_LIMIT - lba) return 0;
    const unsigned char *bytes = (const unsigned char *)buffer;
    for (ed_u64 i = 0; i < count; ++i)
        if (!ata_write_sector(ata, lba + i, bytes + i * 512ULL)) return 0;
    return 1;
}
int edos_ata_pio_init(edos_block_device_t *device, edos_ata_pio_t *ata,
                      unsigned int slave) {
    if (!device || !ata || slave > 1U) return 0;
    ata->present = 0;
    ata->sector_count = 0;
    ata->slave = slave;
    outb(ATA_CONTROL, 0x02U); /* Polling mode; disable device IRQs. */
    outb(ATA_DRIVE, (unsigned char)(0xa0U | (slave << 4)));
    ata_delay_400ns();
    outb(ATA_SECTOR_COUNT, 0U);
    outb(ATA_LBA_LOW, 0U);
    outb(ATA_LBA_MID, 0U);
    outb(ATA_LBA_HIGH, 0U);
    outb(ATA_COMMAND, 0xecU); /* IDENTIFY DEVICE */
    unsigned char status = inb(ATA_STATUS);
    if (status == 0U || status == 0xffU || !ata_wait(1U)) return 0;
    if (inb(ATA_LBA_MID) != 0U || inb(ATA_LBA_HIGH) != 0U) return 0; /* ATAPI/other. */
    unsigned short identify[256];
    for (unsigned int i = 0; i < 256U; ++i) identify[i] = inw(ATA_DATA);
    if (!(identify[49] & (1U << 9))) return 0; /* LBA support required. */
    ed_u64 sectors = (ed_u64)identify[60] | ((ed_u64)identify[61] << 16);
    if (!sectors) return 0;
    if (sectors > ATA_LBA28_LIMIT) sectors = ATA_LBA28_LIMIT;
    ata->sector_count = sectors;
    ata->present = 1;
    if (!edos_block_device_init(device, ata, sectors, 512U,
                                ata_block_read, ata_block_write, 0)) {
        ata->present = 0;
        return 0;
    }
    return 1;
}
