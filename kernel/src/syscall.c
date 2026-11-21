#include "syscall.h"
ed_u64 edos_syscall_dispatch(ed_u64 number, ed_u64 arg0, ed_u64 arg1,
                             edos_syscall_context_t *context) {
    if (!context || context->user_start > context->user_end)
        return EDOS_SYSCALL_EINVAL;
    switch (number) {
    case EDOS_SYSCALL_WRITE: {
        if (arg1 > EDOS_SYSCALL_MAX_WRITE || (!arg0 && arg1))
            return EDOS_SYSCALL_EINVAL;
        if (arg1 == 0) return EDOS_SYSCALL_OK;
        if (arg0 < context->user_start || arg0 > context->user_end ||
            arg1 > context->user_end - arg0)
            return EDOS_SYSCALL_EFAULT;
        if (!context->write) return EDOS_SYSCALL_ENOSYS;
        return context->write((const char *)(unsigned long long)arg0,
                              arg1, context->opaque) ? EDOS_SYSCALL_OK : EDOS_SYSCALL_EFAULT;
    }
    case EDOS_SYSCALL_EXIT:
        if (!context->exit) return EDOS_SYSCALL_ENOSYS;
        context->exit(arg0, context->opaque);
        return EDOS_SYSCALL_OK;
    case EDOS_SYSCALL_YIELD:
        if (!context->yield) return EDOS_SYSCALL_ENOSYS;
        context->yield(context->opaque);
        return EDOS_SYSCALL_OK;
    case EDOS_SYSCALL_GETPID:
        return context->pid;
    default:
        return EDOS_SYSCALL_ENOSYS;
    }
}
