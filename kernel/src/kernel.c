/* E.D-OS kernel 0.0.2 — freestanding x86_64 boot milestone. */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long long u64;

/* Limine base revision 3 tag. The bootloader sets the last value to zero
 * when it supports the requested protocol revision. */
__attribute__((used, section(".limine_requests"), aligned(8)))
volatile u64 limine_base_revision[3] = {
    0xf9562b2d5c95a6c8ULL,
    0x6a7b384944536bdcULL,
    3
};

static inline void outb(u16 port, u8 value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void) {
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x80);
    outb(0x3F8, 0x01);
    outb(0x3F9, 0x00);
    outb(0x3FB, 0x03);
    outb(0x3FA, 0xC7);
    outb(0x3FC, 0x0B);
}

static void serial_write_char(char value) {
    unsigned int timeout = 1000000;
    while ((inb(0x3FD) & 0x20) == 0 && timeout != 0) {
        --timeout;
    }
    if (timeout != 0) {
        outb(0x3F8, (u8)value);
    }
}

static void serial_write(const char *message) {
    while (*message != '\0') {
        if (*message == '\n') serial_write_char('\r');
        serial_write_char(*message++);
    }
}

__attribute__((noreturn))
void kernel_main(void) {
    serial_init();
    serial_write("\nE.D-OS kernel 0.0.2\n");
    serial_write("BOOT_PROTOCOL=Limine\n");
    serial_write("KERNEL_BOOT_OK\n");
    serial_write("Kernel entered in x86_64 long mode.\n");
    serial_write("Next: memory map, physical allocator and IDT.\n");
    for (;;) __asm__ volatile ("cli; hlt");
}
