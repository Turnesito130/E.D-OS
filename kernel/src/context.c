#include "context.h"
int edos_context_init(edos_cpu_context_t *context, void *stack_base,
                      ed_u64 stack_size, void (*entry)(void),
                      void (*return_trap)(void)) {
    if (!context || !stack_base || !entry || !return_trap || stack_size < 256ULL)
        return 0;
    ed_u64 start = (ed_u64)(unsigned long long)stack_base;
    if (stack_size > ~0ULL - start) return 0;
    ed_u64 end = (start + stack_size) & ~15ULL;
    if (end < start || end - start < 16ULL) return 0;
    ed_u64 stack_pointer = end - 8ULL;
    *(ed_u64 *)(unsigned long long)stack_pointer =
        (ed_u64)(unsigned long long)return_trap;
    context->rsp = stack_pointer;
    context->rbx = context->rbp = 0;
    context->r12 = context->r13 = context->r14 = context->r15 = 0;
    context->rip = (ed_u64)(unsigned long long)entry;
    return 1;
}
