/*
 * E.D-OS kernel 0.0.1 — minimal freestanding core.
 * Original E.D-OS code. No Linux headers or runtime dependencies.
 */
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

static inline void outb(u16 port, u8 value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void serial_init(void) {
    const u16 com1 = 0x3F8;
    outb(com1 + 1, 0x00); /* Disable UART interrupts. */
    outb(com1 + 3, 0x80); /* Enable divisor latch. */
    outb(com1 + 0, 0x01); /* 115200 baud. */
    outb(com1 + 1, 0x00);
    outb(com1 + 3, 0x03); /* 8 data bits, no parity, one stop bit. */
    outb(com1 + 2, 0xC7); /* Enable and clear FIFO. */
    outb(com1 + 4, 0x0B); /* Enable IRQs, RTS/DSR. */
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
        if (*message == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*message++);
    }
}

__attribute__((noreturn))
void kernel_main(void) {
    serial_init();
    serial_write("\nE.D-OS kernel 0.0.1\n");
    serial_write("Independent kernel entry reached.\n");
    serial_write("Serial diagnostics initialized.\n");
    serial_write("CPU halted: next milestone is a documented bootloader hand-off.\n");

    for (;;) {
        __asm__ volatile ("cli; hlt");
    }
}
