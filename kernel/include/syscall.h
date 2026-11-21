#ifndef EDOS_SYSCALL_H
#define EDOS_SYSCALL_H
#include "memory.h"
#define EDOS_SYSCALL_WRITE 0ULL
#define EDOS_SYSCALL_EXIT 1ULL
#define EDOS_SYSCALL_YIELD 2ULL
#define EDOS_SYSCALL_GETPID 3ULL
#define EDOS_SYSCALL_MAX_WRITE 4096ULL
#define EDOS_SYSCALL_OK 0ULL
#define EDOS_SYSCALL_EINVAL (~0ULL)
#define EDOS_SYSCALL_ENOSYS (~1ULL)
#define EDOS_SYSCALL_EFAULT (~2ULL)
typedef struct {
    ed_u64 user_start;
    ed_u64 user_end;
    ed_u64 pid;
    int (*write)(const char *buffer, ed_u64 length, void *opaque);
    void (*yield)(void *opaque);
    void (*exit)(ed_u64 status, void *opaque);
    void *opaque;
} edos_syscall_context_t;
ed_u64 edos_syscall_dispatch(ed_u64 number, ed_u64 arg0, ed_u64 arg1,
                             edos_syscall_context_t *context);
#endif
