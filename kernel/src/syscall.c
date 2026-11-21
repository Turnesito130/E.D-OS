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
        if (context->exit) {
            context->exit(arg0, context->opaque);
            return EDOS_SYSCALL_OK;
        }
        if (context->scheduler && context->scheduler->current_id)
            return edos_task_set_state(context->scheduler,
                context->scheduler->current_id, EDOS_TASK_EXITED)
                ? EDOS_SYSCALL_OK : EDOS_SYSCALL_EFAULT;
        return EDOS_SYSCALL_ENOSYS;
    case EDOS_SYSCALL_YIELD:
        if (context->yield) {
            context->yield(context->opaque);
            return EDOS_SYSCALL_OK;
        }
        if (context->scheduler) {
            edos_scheduler_t *scheduler = context->scheduler;
            if (scheduler->current_id) {
                edos_task_t *current = (edos_task_t *)0;
                for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i)
                    if (scheduler->tasks[i].id == scheduler->current_id &&
                        scheduler->tasks[i].state == EDOS_TASK_RUNNING) {
                        current = &scheduler->tasks[i];
                        break;
                    }
                if (current && current->context_ready) {
                    edos_task_t *next = edos_scheduler_pick(scheduler);
                    if (!next || !next->context_ready) return EDOS_SYSCALL_EAGAIN;
                    return edos_scheduler_switch_to(scheduler, current->id, next->id)
                        ? EDOS_SYSCALL_OK : EDOS_SYSCALL_EFAULT;
                }
            }
            return edos_scheduler_schedule(scheduler)
                ? EDOS_SYSCALL_OK : EDOS_SYSCALL_EAGAIN;
        }
        return EDOS_SYSCALL_ENOSYS;
    case EDOS_SYSCALL_GETPID:
        return context->pid ? context->pid :
            (context->scheduler ? context->scheduler->current_id : 0);
    default:
        return EDOS_SYSCALL_ENOSYS;
    }
}
