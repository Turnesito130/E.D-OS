#include "irq.h"
#include "keyboard.h"
typedef unsigned char u8;
typedef unsigned short u16;
static volatile ed_u64 timer_ticks;
static inline void outb(u16 port, u8 value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}
static inline void io_wait(void) { outb(0x80U, 0); }

int edos_irq_init(int enable_keyboard) {
    timer_ticks = 0;
    /* Remap the legacy PIC to vectors 0x20-0x2f. */
    outb(0x20U, 0x11U); io_wait();
    outb(0xa0U, 0x11U); io_wait();
    outb(0x21U, 0x20U); io_wait();
    outb(0xa1U, 0x28U); io_wait();
    outb(0x21U, 0x04U); io_wait();
    outb(0xa1U, 0x02U); io_wait();
    outb(0x21U, 0x01U); io_wait();
    outb(0xa1U, 0x01U); io_wait();
    /* Only IRQ0 (PIT timer) is unmasked; all other IRQs remain disabled. */
    outb(0x21U, enable_keyboard ? 0xfcU : 0xfeU);
    outb(0xa1U, 0xffU);
    /* PIT channel 0, square-wave mode, approximately 100 Hz. */
    const u16 divisor = 11931U;
    outb(0x43U, 0x36U);
    outb(0x40U, (u8)(divisor & 0xffU));
    outb(0x40U, (u8)(divisor >> 8));
    __asm__ volatile ("sti" : : : "memory");
    return 1;
}

void irq_dispatch(ed_u64 vector) {
    if (vector == 32U) ++timer_ticks;
    if (vector == 33U) edos_keyboard_handle_irq();
    if (vector >= 40U) outb(0xa0U, 0x20U);
    outb(0x20U, 0x20U);
}

ed_u64 edos_timer_ticks(void) {
    return timer_ticks;
}
