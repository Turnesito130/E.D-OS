#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "syscall.h"

typedef struct { char output[32]; ed_u64 length; ed_u64 exit_status; unsigned int yields; } test_state;
static int capture(const char *buffer, ed_u64 length, void *opaque) {
    test_state *state = opaque;
    if (!buffer || length > sizeof(state->output)) return 0;
    memcpy(state->output, buffer, (size_t)length);
    state->length = length;
    return 1;
}
static void do_yield(void *opaque) { ((test_state *)opaque)->yields++; }
static void do_exit(ed_u64 status, void *opaque) { ((test_state *)opaque)->exit_status = status; }

int main(void) {
    const char message[] = "E.D-OS";
    test_state state = {0};
    edos_syscall_context_t context = {
        .user_start = (ed_u64)(unsigned long long)message,
        .user_end = (ed_u64)(unsigned long long)(message + sizeof(message)),
        .pid = 42, .write = capture, .yield = do_yield, .exit = do_exit,
        .opaque = &state
    };
    assert(edos_syscall_dispatch(EDOS_SYSCALL_GETPID, 0, 0, &context) == 42);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_WRITE,
        (ed_u64)(unsigned long long)message, 6, &context) == EDOS_SYSCALL_OK);
    assert(state.length == 6 && memcmp(state.output, "E.D-OS", 6) == 0);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_WRITE,
        context.user_end - 2, 4, &context) == EDOS_SYSCALL_EFAULT);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_WRITE,
        context.user_start, EDOS_SYSCALL_MAX_WRITE + 1, &context) == EDOS_SYSCALL_EINVAL);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_YIELD, 0, 0, &context) == EDOS_SYSCALL_OK);
    assert(state.yields == 1);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_EXIT, 7, 0, &context) == EDOS_SYSCALL_OK);
    assert(state.exit_status == 7);
    assert(edos_syscall_dispatch(99, 0, 0, &context) == EDOS_SYSCALL_ENOSYS);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_GETPID, 0, 0, 0) == EDOS_SYSCALL_EINVAL);
    edos_syscall_context_t invalid = context;
    invalid.user_start = invalid.user_end + 1;
    assert(edos_syscall_dispatch(EDOS_SYSCALL_WRITE, invalid.user_start, 1, &invalid) == EDOS_SYSCALL_EINVAL);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_WRITE, 0, 0, &context) == EDOS_SYSCALL_OK);
    puts("SYSCALL_TESTS_OK");
    return 0;
}
