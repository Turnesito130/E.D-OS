/* E.D-OS kernel 0.0.3 — memory discovery and first page allocation. */
#include "memory.h"
#include "idt.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long long u64;

__attribute__((used, section(".limine_requests"), aligned(8)))
volatile u64 limine_base_revision[3] = {
    0xf9562b2d5c95a6c8ULL, 0x6a7b384944536bdcULL, 3
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
    outb(0x3F9, 0x00); outb(0x3FB, 0x80);
    outb(0x3F8, 0x01); outb(0x3F9, 0x00);
    outb(0x3FB, 0x03); outb(0x3FA, 0xC7); outb(0x3FC, 0x0B);
}
static void serial_write_char(char value) {
    unsigned int timeout = 1000000;
    while ((inb(0x3FD) & 0x20) == 0 && timeout != 0) --timeout;
    if (timeout != 0) outb(0x3F8, (u8)value);
}
static void serial_write(const char *message) {
    while (*message != '\0') {
        if (*message == '\n') serial_write_char('\r');
        serial_write_char(*message++);
    }
}
static void serial_hex(u64 value) {
    static const char digits[] = "0123456789abcdef";
    serial_write("0x");
    for (int shift = 60; shift >= 0; shift -= 4)
        serial_write_char(digits[(value >> shift) & 0xf]);
}
__attribute__((noreturn))
void kernel_main(void) {
    serial_init();
    serial_write("\nE.D-OS kernel 0.0.3\n");
    if (limine_base_revision[2] != 0) {
        serial_write("BOOT_ERROR: Limine base revision 3 not confirmed.\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }
    serial_write("BOOT_PROTOCOL=Limine revision 3\n");
    if (!memory_init()) {
        serial_write("MEMORY_ERROR: missing or invalid Limine memory map/HHDM response.\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }
    if (!idt_init()) {
        serial_write("IDT_ERROR: could not install exception table.\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }
    serial_write("IDT_READY entries=256 exceptions=32\n");

    serial_write("MEMORY_MAP_OK usable_bytes=");
    serial_hex(memory_usable_bytes());
    serial_write(" hhdm=");
    serial_hex(memory_hhdm_offset());
    serial_write("\n");

    void *first_page = pmm_alloc_page();
    if (first_page == (void *)0) {
        serial_write("PMM_ERROR: no usable 4 KiB page available.\n");
        for (;;) __asm__ volatile ("cli; hlt");
    }
    serial_write("PMM_FIRST_PAGE_ZEROED virtual=");
    serial_hex((u64)(unsigned long long)first_page);
    serial_write(" allocated_pages=");
    serial_hex(pmm_pages_allocated());
    serial_write("\nKERNEL_MEMORY_MILESTONE_OK\n");
    serial_write("TEST: triggering breakpoint exception (vector 3).\n");
    __asm__ volatile ("int3");

    for (;;) __asm__ volatile ("cli; hlt");
}

/* Called by the assembly exception stubs. No exception is resumed yet. */
__attribute__((noreturn))
void kernel_exception_panic(u64 vector, u64 error_code) {
    serial_write("EXCEPTION vector=");
    serial_hex(vector);
    serial_write(" error=");
    serial_hex(error_code);
    serial_write("\nKERNEL_HALTED_ON_EXCEPTION\n");
    for (;;) __asm__ volatile ("cli; hlt");
}
