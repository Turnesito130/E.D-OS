/* x86_64 interrupt descriptor table for E.D-OS. */
#include "idt.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef struct __attribute__((packed)) {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 attributes;
    u16 offset_middle;
    u32 offset_high;
    u32 reserved;
} idt_gate;

typedef struct __attribute__((packed)) {
    u16 limit;
    u64 base;
} idtr;

extern void isr_unexpected(void);
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);

static idt_gate idt[256] __attribute__((aligned(16)));

static void idt_set_gate(u8 vector, void (*handler)(void), u16 selector) {
    u64 address = (u64)(unsigned long long)handler;
    idt[vector].offset_low = (u16)(address & 0xffff);
    idt[vector].selector = selector;
    idt[vector].ist = 0;
    idt[vector].attributes = 0x8e; /* present, ring 0, 64-bit interrupt gate */
    idt[vector].offset_middle = (u16)((address >> 16) & 0xffff);
    idt[vector].offset_high = (u32)(address >> 32);
    idt[vector].reserved = 0;
}

int idt_init(void) {
    u16 code_selector;
    __asm__ volatile ("mov %%cs, %0" : "=r"(code_selector));
    if (code_selector == 0) return 0;

    for (u32 i = 0; i < 256; ++i)
        idt_set_gate((u8)i, isr_unexpected, code_selector);

    void (* const exceptions[32])(void) = {
        isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7,
        isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };
    for (u32 i = 0; i < 32; ++i)
        idt_set_gate((u8)i, exceptions[i], code_selector);

    idtr descriptor = {
        .limit = (u16)(sizeof(idt) - 1),
        .base = (u64)(unsigned long long)&idt[0]
    };
    __asm__ volatile ("lidt %0" : : "m"(descriptor) : "memory");
    return 1;
}
