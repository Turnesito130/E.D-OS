#include "gdt.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef struct __attribute__((packed)) {
    u32 reserved0;
    u64 rsp0, rsp1, rsp2;
    u64 reserved1;
    u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
    u64 reserved2;
    u16 reserved3;
    u16 iomap_base;
} edos_tss_t;
typedef struct __attribute__((packed)) { u16 limit; u64 base; } edos_gdtr_t;
extern void edos_gdt_load(const edos_gdtr_t *descriptor);
extern char kernel_stack_top[];
static u64 gdt[7] __attribute__((aligned(16)));
static edos_tss_t tss __attribute__((aligned(16)));
static u8 double_fault_stack[8192] __attribute__((aligned(16)));
static u8 nmi_stack[8192] __attribute__((aligned(16)));
static u8 page_fault_stack[8192] __attribute__((aligned(16)));
static u64 make_tss_low(u64 base, u32 limit) {
    return ((u64)(limit & 0xffffU)) |
           ((base & 0xffffffULL) << 16) |
           (0x89ULL << 40) |
           ((u64)((limit >> 16) & 0x0fU) << 48) |
           (((base >> 24) & 0xffULL) << 56);
}
int edos_gdt_init(void) {
    gdt[0] = 0;
    gdt[1] = 0x00af9a000000ffffULL; /* Ring 0, 64-bit code. */
    gdt[2] = 0x00cf92000000ffffULL; /* Ring 0 data. */
    gdt[3] = 0x00cff2000000ffffULL; /* Ring 3 data. */
    gdt[4] = 0x00affa000000ffffULL; /* Ring 3, 64-bit code. */
    tss.reserved0 = 0;
    tss.rsp0 = (u64)(unsigned long long)kernel_stack_top;
    tss.rsp1 = tss.rsp2 = 0;
    tss.reserved1 = 0;
    tss.ist1 = ((u64)(unsigned long long)(double_fault_stack + sizeof(double_fault_stack))) & ~15ULL;
    tss.ist2 = ((u64)(unsigned long long)(nmi_stack + sizeof(nmi_stack))) & ~15ULL;
    tss.ist3 = ((u64)(unsigned long long)(page_fault_stack + sizeof(page_fault_stack))) & ~15ULL;
    tss.ist4 = tss.ist5 = tss.ist6 = tss.ist7 = 0;
    tss.reserved2 = 0;
    tss.reserved3 = 0;
    tss.iomap_base = (u16)sizeof(tss);
    u64 base = (u64)(unsigned long long)&tss;
    u32 limit = (u32)sizeof(tss) - 1U;
    gdt[5] = make_tss_low(base, limit);
    gdt[6] = base >> 32;
    edos_gdtr_t descriptor = {
        .limit = (u16)(sizeof(gdt) - 1U),
        .base = (u64)(unsigned long long)&gdt[0]
    };
    edos_gdt_load(&descriptor);
    u16 task_register;
    __asm__ volatile ("str %0" : "=r"(task_register));
    return task_register == 0x28U;
}

int edos_tss_set_rsp0(void *stack_top) {
    u64 address = (u64)(unsigned long long)stack_top;
    u64 upper = address >> 48;
    if (!address || (address & 15ULL) || (upper != 0ULL && upper != 0xffffULL))
        return 0;
    tss.rsp0 = address;
    return 1;
}
