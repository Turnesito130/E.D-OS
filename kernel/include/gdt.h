#ifndef EDOS_GDT_H
#define EDOS_GDT_H
int edos_gdt_init(void);
int edos_tss_set_rsp0(void *stack_top);
#endif
