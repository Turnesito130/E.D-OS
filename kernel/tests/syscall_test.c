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
static void task_one(void) {}
static void task_two(void) {}
static edos_scheduler_t yield_scheduler;
static edos_cpu_context_t yield_main_context;
static ed_u64 yield_task_a_id, yield_task_b_id;
static unsigned char yield_stack_a[4096] __attribute__((aligned(16)));
static unsigned char yield_stack_b[4096] __attribute__((aligned(16)));
static volatile int yield_stage;
static void yield_return_trap(void) { for (;;) {} }
static void yield_task_a(void) {
    yield_stage = 1;
    edos_syscall_context_t context = { .scheduler = &yield_scheduler };
    assert(edos_syscall_dispatch(EDOS_SYSCALL_YIELD, 0, 0, &context) == EDOS_SYSCALL_OK);
    assert(yield_stage == 2);
    yield_stage = 3;
    edos_context_switch(&yield_scheduler.tasks[0].context, &yield_main_context);
    for (;;) {}
}
static void yield_task_b(void) {
    assert(yield_stage == 1);
    yield_stage = 2;
    edos_syscall_context_t context = { .scheduler = &yield_scheduler };
    assert(edos_syscall_dispatch(EDOS_SYSCALL_YIELD, 0, 0, &context) == EDOS_SYSCALL_OK);
    for (;;) {}
}

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
    edos_scheduler_t scheduler;
    edos_scheduler_init(&scheduler);
    ed_u64 first = edos_task_create(&scheduler, task_one, 0);
    ed_u64 second = edos_task_create(&scheduler, task_two, 0);
    assert(first && second);
    assert(edos_scheduler_schedule(&scheduler)->id == first);
    edos_syscall_context_t integrated = { .scheduler = &scheduler };
    assert(edos_syscall_dispatch(EDOS_SYSCALL_GETPID, 0, 0, &integrated) == first);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_YIELD, 0, 0, &integrated) == EDOS_SYSCALL_OK);
    assert(scheduler.current_id == second);
    assert(edos_syscall_dispatch(EDOS_SYSCALL_EXIT, 0, 0, &integrated) == EDOS_SYSCALL_OK);
    assert(scheduler.tasks[1].state == EDOS_TASK_EXITED);
    assert(edos_scheduler_validate(&scheduler));
    assert(edos_syscall_dispatch(EDOS_SYSCALL_YIELD, 0, 0, &integrated) == EDOS_SYSCALL_OK);
    assert(scheduler.current_id == first);

    edos_scheduler_init(&yield_scheduler);
    yield_task_a_id = edos_task_create(&yield_scheduler, yield_task_a, yield_stack_a);
    yield_task_b_id = edos_task_create(&yield_scheduler, yield_task_b, yield_stack_b);
    assert(yield_task_a_id && yield_task_b_id);
    assert(edos_task_prepare_context(&yield_scheduler, yield_task_a_id, yield_stack_a, sizeof(yield_stack_a), yield_return_trap));
    assert(edos_task_prepare_context(&yield_scheduler, yield_task_b_id, yield_stack_b, sizeof(yield_stack_b), yield_return_trap));
    assert(edos_task_set_state(&yield_scheduler, yield_task_a_id, EDOS_TASK_RUNNING));
    edos_context_switch(&yield_main_context, &yield_scheduler.tasks[0].context);
    assert(yield_stage == 3);
    assert(yield_scheduler.current_id == yield_task_a_id);
    assert(yield_scheduler.tasks[0].state == EDOS_TASK_RUNNING);
    assert(yield_scheduler.tasks[1].state == EDOS_TASK_READY);
    assert(edos_scheduler_validate(&yield_scheduler));
    puts("SYSCALL_TESTS_OK");
    return 0;
}
