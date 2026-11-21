#include <assert.h>
#include <stdio.h>
#include "scheduler.h"

static unsigned char task_stack[4096] __attribute__((aligned(16)));
static void task_returned(void) { for (;;) {} }
static void task_a(void) {}
static void task_b(void) {}

static edos_scheduler_t run_scheduler;
static edos_cpu_context_t main_context;
static ed_u64 run_a, run_b;
static unsigned char run_stack_a[4096] __attribute__((aligned(16)));
static unsigned char run_stack_b[4096] __attribute__((aligned(16)));
static volatile int run_stage;

static void cooperative_a(void) {
    run_stage = 1;
    assert(edos_scheduler_switch_to(&run_scheduler, run_a, run_b));
    assert(run_stage == 2);
    run_stage = 3;
    edos_context_switch(&run_scheduler.tasks[0].context, &main_context);
    for (;;) {}
}
static void cooperative_b(void) {
    assert(run_stage == 1);
    run_stage = 2;
    assert(edos_scheduler_switch_to(&run_scheduler, run_b, run_a));
    for (;;) {}
}

int main(void) {
    edos_scheduler_t s;
    edos_scheduler_init(&s);
    assert(edos_scheduler_validate(&s));
    ed_u64 a = edos_task_create(&s, task_a, (void *)1);
    ed_u64 b = edos_task_create(&s, task_b, (void *)2);
    assert(a && b && a != b);
    assert(edos_task_prepare_context(&s, a, task_stack, sizeof(task_stack), task_returned));
    assert(s.tasks[0].context_ready);
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
    edos_task_t *reused = edos_scheduler_schedule(&s);
    assert(reused && reused->state == EDOS_TASK_RUNNING);
    assert(edos_task_set_state(&s, reused->id, EDOS_TASK_EXITED));
    assert(edos_scheduler_schedule(&s) == 0);
    assert(edos_scheduler_validate(&s));

    edos_scheduler_init(&run_scheduler);
    run_a = edos_task_create(&run_scheduler, cooperative_a, run_stack_a);
    run_b = edos_task_create(&run_scheduler, cooperative_b, run_stack_b);
    assert(run_a && run_b);
    assert(edos_task_prepare_context(&run_scheduler, run_a, run_stack_a, sizeof(run_stack_a), task_returned));
    assert(edos_task_prepare_context(&run_scheduler, run_b, run_stack_b, sizeof(run_stack_b), task_returned));
    assert(edos_task_set_state(&run_scheduler, run_a, EDOS_TASK_RUNNING));
    edos_context_switch(&main_context, &run_scheduler.tasks[0].context);
    assert(run_stage == 3);
    assert(run_scheduler.current_id == run_a);
    assert(run_scheduler.tasks[0].state == EDOS_TASK_RUNNING);
    assert(run_scheduler.tasks[1].state == EDOS_TASK_READY);
    assert(edos_scheduler_validate(&run_scheduler));
    /* Verify that timer IRQ frames rotate through interrupt-ready tasks. */
    static unsigned char irq_stack_a[4096] __attribute__((aligned(16)));
    static unsigned char irq_stack_b[4096] __attribute__((aligned(16)));
    static unsigned char irq_stack_c[4096] __attribute__((aligned(16)));
    static ed_u64 saved_frame_a[20], saved_frame_b[20], saved_frame_c[20];
    edos_scheduler_t preempt;
    edos_scheduler_init(&preempt);
    ed_u64 pa = edos_task_create(&preempt, task_a, irq_stack_a);
    ed_u64 pb = edos_task_create(&preempt, task_b, irq_stack_b);
    ed_u64 pc = edos_task_create(&preempt, task_a, irq_stack_c);
    assert(pa && pb && pc);
    void *initial_a = edos_task_prepare_interrupt_context(&preempt, pa, irq_stack_a,
                                                           sizeof(irq_stack_a), task_returned);
    void *initial_b = edos_task_prepare_interrupt_context(&preempt, pb, irq_stack_b,
                                                           sizeof(irq_stack_b), task_returned);
    void *initial_c = edos_task_prepare_interrupt_context(&preempt, pc, irq_stack_c,
                                                           sizeof(irq_stack_c), task_returned);
    assert(initial_a && initial_b && initial_c);
    ed_u64 *initial_words = (ed_u64 *)initial_a;
    assert(initial_words[17] == (ed_u64)(unsigned long long)task_a);
    assert(initial_words[18] == 0x08U && initial_words[19] == 0x202U);
    assert(edos_scheduler_validate(&preempt));
    assert(edos_scheduler_start_interrupt(&preempt, pa) == initial_a);
    assert(preempt.current_id == pa);
    assert(edos_scheduler_timer_switch(saved_frame_a) == initial_b);
    assert(preempt.tasks[0].interrupt_frame == saved_frame_a);
    assert(preempt.current_id == pb);
    assert(edos_scheduler_timer_switch(saved_frame_b) == initial_c);
    assert(preempt.tasks[1].interrupt_frame == saved_frame_b);
    assert(preempt.current_id == pc);
    assert(edos_scheduler_timer_switch(saved_frame_c) == saved_frame_a);
    assert(preempt.tasks[2].interrupt_frame == saved_frame_c);
    assert(preempt.current_id == pa);
    assert(preempt.preemptive_switches == 3U);
    assert(edos_scheduler_validate(&preempt));

    puts("SCHEDULER_TESTS_OK");
    return 0;
}
