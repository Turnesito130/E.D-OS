#include <assert.h>
#include <stdio.h>
#include "scheduler.h"
static void task_a(void) {}
static void task_b(void) {}
int main(void) {
    edos_scheduler_t s;
    edos_scheduler_init(&s);
    assert(edos_scheduler_validate(&s));
    ed_u64 a = edos_task_create(&s, task_a, (void *)1);
    ed_u64 b = edos_task_create(&s, task_b, (void *)2);
    assert(a && b && a != b);
    assert(edos_scheduler_validate(&s));
    edos_task_t *current = edos_scheduler_schedule(&s);
    assert(current && current->id == a && current->state == EDOS_TASK_RUNNING);
    assert(!edos_task_set_state(&s, b, EDOS_TASK_RUNNING));
    assert(edos_scheduler_validate(&s));
    current = edos_scheduler_schedule(&s);
    assert(current && current->id == b && current->state == EDOS_TASK_RUNNING);
    assert(s.tasks[0].state == EDOS_TASK_READY);
    assert(edos_task_set_state(&s, a, EDOS_TASK_BLOCKED));
    assert(edos_scheduler_validate(&s));
    current = edos_scheduler_schedule(&s);
    assert(current && current->id == b);
    assert(edos_task_set_state(&s, b, EDOS_TASK_EXITED));
    assert(edos_scheduler_validate(&s));
    assert(edos_task_create(&s, task_a, 0) != 0);
    assert(edos_scheduler_validate(&s));
    assert(edos_task_set_state(&s, a, EDOS_TASK_EXITED));
    assert(edos_scheduler_schedule(&s) == 0);
    assert(edos_scheduler_validate(&s));
    puts("SCHEDULER_TESTS_OK");
    return 0;
}
