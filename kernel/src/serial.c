#include "serial.h"
typedef unsigned char u8;
typedef unsigned short u16;
static inline void outb(u16 port, u8 value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static inline u8 inb(u16 port) {
    u8 value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
#define COM1 0x3F8U
int edos_serial_init(void) {
    outb(COM1 + 1U, 0x00U);
    outb(COM1 + 3U, 0x80U);
    outb(COM1 + 0U, 0x01U);
    outb(COM1 + 1U, 0x00U);
    outb(COM1 + 3U, 0x03U);
    outb(COM1 + 2U, 0xC7U);
    outb(COM1 + 4U, 0x0BU);
    return 1; /* UART configurada; disponibilidad se comprueba en cada escritura. */
}
int edos_serial_write_char(char value) {
    unsigned int timeout = 1000000U;
    while (!(inb(COM1 + 5U) & 0x20U) && timeout) --timeout;
    if (!timeout) return 0;
    outb(COM1, (u8)value);
    return 1;
}
size_t edos_serial_write(const char *text) {
    size_t written = 0;
    if (!text) return 0;
    while (*text) {
        if (*text == '\n' && !edos_serial_write_char('\r')) break;
        if (!edos_serial_write_char(*text++)) break;
        ++written;
    }
    return written;
}
