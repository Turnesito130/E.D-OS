#ifndef EDOS_CONTEXT_H
#define EDOS_CONTEXT_H
#include "memory.h"
typedef struct {
    ed_u64 rsp;
    ed_u64 rbx;
    ed_u64 rbp;
    ed_u64 r12;
    ed_u64 r13;
    ed_u64 r14;
    ed_u64 r15;
    ed_u64 rip;
} edos_cpu_context_t;
int edos_context_init(edos_cpu_context_t *context, void *stack_base,
                      ed_u64 stack_size, void (*entry)(void),
                      void (*return_trap)(void));
void edos_context_switch(edos_cpu_context_t *from, const edos_cpu_context_t *to);
#endif
