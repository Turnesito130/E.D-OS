#include "scheduler.h"

static edos_scheduler_t *active_interrupt_scheduler;

static edos_task_t *find_task(edos_scheduler_t *s, ed_u64 id) {
    if (!s || !id) return (edos_task_t *)0;
    for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i)
        if (s->tasks[i].id == id && s->tasks[i].state != EDOS_TASK_UNUSED)
            return &s->tasks[i];
    return (edos_task_t *)0;
}

void edos_scheduler_init(edos_scheduler_t *s) {
    if (!s) return;
    if (active_interrupt_scheduler == s) active_interrupt_scheduler = (edos_scheduler_t *)0;
    for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i) {
        s->tasks[i].id = 0;
        s->tasks[i].state = EDOS_TASK_UNUSED;
        s->tasks[i].entry = (void (*)(void))0;
        s->tasks[i].stack = (void *)0;
        s->tasks[i].context_ready = 0;
        s->tasks[i].interrupt_frame = (void *)0;
        s->tasks[i].interrupt_context_ready = 0;
    }
    s->next_id = 1;
    s->cursor = 0;
    s->current_id = 0;
    s->preemptive_switches = 0;
    s->initialized = 1;
}

ed_u64 edos_task_create(edos_scheduler_t *s, void (*entry)(void), void *stack) {
    if (!s || !s->initialized || !entry) return 0;
    for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i) {
        edos_task_t *t = &s->tasks[i];
        if (t->state == EDOS_TASK_UNUSED || t->state == EDOS_TASK_EXITED) {
            ed_u64 id = s->next_id++;
            if (!id) id = s->next_id++;
            if (!s->next_id) s->next_id = 1;
            t->id = id;
            t->entry = entry;
            t->stack = stack;
            t->context_ready = 0;
            t->interrupt_frame = (void *)0;
            t->interrupt_context_ready = 0;
            t->state = EDOS_TASK_READY;
            return id;
        }
    }
    return 0;
}

int edos_task_set_state(edos_scheduler_t *s, ed_u64 id, edos_task_state_t state) {
    if (!s || !s->initialized || !id ||
        state < EDOS_TASK_READY || state > EDOS_TASK_EXITED) return 0;
    edos_task_t *task = find_task(s, id);
    if (!task) return 0;
    if (state == EDOS_TASK_RUNNING) {
        for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i)
            if (s->tasks[i].id != id && s->tasks[i].state == EDOS_TASK_RUNNING)
                return 0;
        s->current_id = id;
    } else if (s->current_id == id) {
        s->current_id = 0;
    }
    task->state = state;
    return 1;
}

edos_task_t *edos_scheduler_pick(edos_scheduler_t *s) {
    if (!s || !s->initialized) return (edos_task_t *)0;
    for (unsigned int n = 0; n < EDOS_SCHEDULER_MAX_TASKS; ++n) {
        unsigned int i = (unsigned int)((s->cursor + n) % EDOS_SCHEDULER_MAX_TASKS);
        if (s->tasks[i].state == EDOS_TASK_READY) {
            s->cursor = (i + 1U) % EDOS_SCHEDULER_MAX_TASKS;
            return &s->tasks[i];
        }
    }
    return (edos_task_t *)0;
}

/* Legacy round-robin selection API. Use edos_scheduler_switch_to for an actual cooperative CPU context switch. */
edos_task_t *edos_scheduler_schedule(edos_scheduler_t *s) {
    if (!s || !s->initialized) return (edos_task_t *)0;
    edos_task_t *previous = find_task(s, s->current_id);
    if (previous && previous->state == EDOS_TASK_RUNNING)
        previous->state = EDOS_TASK_READY;
    s->current_id = 0;
    edos_task_t *next = edos_scheduler_pick(s);
    if (!next) return (edos_task_t *)0;
    next->state = EDOS_TASK_RUNNING;
    s->current_id = next->id;
    return next;
}

int edos_scheduler_validate(const edos_scheduler_t *s) {
    if (!s || !s->initialized || s->cursor >= EDOS_SCHEDULER_MAX_TASKS ||
        !s->next_id) return 0;
    unsigned int running = 0;
    ed_u64 running_id = 0;
    for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i) {
        const edos_task_t *t = &s->tasks[i];
        if (t->state < EDOS_TASK_UNUSED || t->state > EDOS_TASK_EXITED) return 0;
        if (t->state == EDOS_TASK_UNUSED) {
            if (t->id != 0) return 0;
            continue;
        }
        if (!t->id || !t->entry || (t->context_ready != 0 && t->context_ready != 1) ||
            (t->interrupt_context_ready != 0 && t->interrupt_context_ready != 1) ||
            (t->interrupt_context_ready && !t->interrupt_frame)) return 0;
        for (unsigned int j = i + 1; j < EDOS_SCHEDULER_MAX_TASKS; ++j)
            if (s->tasks[j].state != EDOS_TASK_UNUSED &&
                s->tasks[j].id == t->id) return 0;
        if (t->state == EDOS_TASK_RUNNING) {
            ++running;
            running_id = t->id;
        }
    }
    return running <= 1 && ((running == 0 && s->current_id == 0) ||
                            (running == 1 && s->current_id == running_id));
}

void *edos_task_prepare_interrupt_context(edos_scheduler_t *s, ed_u64 id,
                                          void *stack_base, ed_u64 stack_size,
                                          void (*return_trap)(void)) {
    edos_task_t *task = find_task(s, id);
    if (!task || task->state != EDOS_TASK_READY || !task->entry ||
        !stack_base || !return_trap || stack_size < 256U) return (void *)0;
    ed_u64 base = (ed_u64)(unsigned long long)stack_base;
    if (base > ~0ULL - stack_size) return (void *)0;
    ed_u64 top = (base + stack_size) & ~15ULL;
    if (top < base + 184U) return (void *)0;

    /* Long-mode IRETQ restores RIP/CS/RFLAGS/RSP/SS after the saved GPRs. */
    ed_u64 frame_address = top - 184U;
    ed_u64 *frame = (ed_u64 *)(unsigned long long)frame_address;
    for (unsigned int i = 0; i < 22U; ++i) frame[i] = 0;
    frame[17] = (ed_u64)(unsigned long long)task->entry;
    frame[18] = 0x08U;   /* kernel code selector */
    frame[19] = 0x202U;  /* reserved bit + interrupts enabled */
    frame[20] = top - 8U; /* initial kernel stack pointer */
    frame[21] = 0x10U;    /* kernel data selector */
    *((ed_u64 *)(unsigned long long)(top - 8U)) =
        (ed_u64)(unsigned long long)return_trap;

    task->stack = stack_base;
    task->interrupt_frame = (void *)(unsigned long long)frame_address;
    task->interrupt_context_ready = 1;
    return task->interrupt_frame;
}

void *edos_scheduler_start_interrupt(edos_scheduler_t *s, ed_u64 id) {
    if (!s || !s->initialized || !id) return (void *)0;
    edos_task_t *task = find_task(s, id);
    if (!task || task->state != EDOS_TASK_READY ||
        !task->interrupt_context_ready || !task->interrupt_frame)
        return (void *)0;
    for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i)
        if (s->tasks[i].state == EDOS_TASK_RUNNING) return (void *)0;
    task->state = EDOS_TASK_RUNNING;
    s->current_id = id;
    s->cursor = 0;
    for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i)
        if (s->tasks[i].id == id) { s->cursor = (i + 1U) % EDOS_SCHEDULER_MAX_TASKS; break; }
    s->preemptive_switches = 0;
    active_interrupt_scheduler = s;
    return task->interrupt_frame;
}

void *edos_scheduler_timer_switch(void *interrupt_frame) {
    edos_scheduler_t *s = active_interrupt_scheduler;
    if (!s || !s->initialized || !interrupt_frame) return interrupt_frame;
    edos_task_t *current = find_task(s, s->current_id);
    if (!current || !current->interrupt_context_ready) return interrupt_frame;

    current->interrupt_frame = interrupt_frame;
    current->state = EDOS_TASK_READY;
    edos_task_t *next = (edos_task_t *)0;
    for (unsigned int n = 0; n < EDOS_SCHEDULER_MAX_TASKS; ++n) {
        unsigned int i = (unsigned int)((s->cursor + n) % EDOS_SCHEDULER_MAX_TASKS);
        edos_task_t *candidate = &s->tasks[i];
        if (candidate->state == EDOS_TASK_READY &&
            candidate->interrupt_context_ready && candidate->interrupt_frame) {
            next = candidate;
            s->cursor = (i + 1U) % EDOS_SCHEDULER_MAX_TASKS;
            break;
        }
    }
    if (!next) {
        current->state = EDOS_TASK_RUNNING;
        s->current_id = current->id;
        return interrupt_frame;
    }
    next->state = EDOS_TASK_RUNNING;
    s->current_id = next->id;
    if (next != current) ++s->preemptive_switches;
    return next->interrupt_frame;
}

int edos_task_prepare_context(edos_scheduler_t *s, ed_u64 id,
                              void *stack_base, ed_u64 stack_size,
                              void (*return_trap)(void)) {
    edos_task_t *task = find_task(s, id);
    if (!task || task->state != EDOS_TASK_READY || !task->entry ||
        !edos_context_init(&task->context, stack_base, stack_size,
                           task->entry, return_trap))
        return 0;
    task->stack = stack_base;
    task->context_ready = 1;
    return 1;
}

int edos_scheduler_switch_to(edos_scheduler_t *s, ed_u64 from_id, ed_u64 to_id) {
    if (!s || !s->initialized || !from_id || !to_id || from_id == to_id ||
        s->current_id != from_id) return 0;
    edos_task_t *from = find_task(s, from_id);
    edos_task_t *to = find_task(s, to_id);
    if (!from || !to || !from->context_ready || !to->context_ready ||
        from->state != EDOS_TASK_RUNNING || to->state != EDOS_TASK_READY)
        return 0;
    from->state = EDOS_TASK_READY;
    to->state = EDOS_TASK_RUNNING;
    s->current_id = to_id;
    edos_context_switch(&from->context, &to->context);
    return 1;
}

int edos_scheduler_block_current(edos_scheduler_t *s, ed_u64 from_id, ed_u64 to_id) {
    if (!s || !s->initialized || !from_id || !to_id || from_id == to_id ||
        s->current_id != from_id) return 0;
    edos_task_t *from = find_task(s, from_id);
    edos_task_t *to = find_task(s, to_id);
    if (!from || !to || !from->context_ready || !to->context_ready ||
        from->state != EDOS_TASK_RUNNING || to->state != EDOS_TASK_READY)
        return 0;
    from->state = EDOS_TASK_BLOCKED;
    to->state = EDOS_TASK_RUNNING;
    s->current_id = to_id;
    edos_context_switch(&from->context, &to->context);
    return 1;
}

int edos_scheduler_wake(edos_scheduler_t *s, ed_u64 task_id) {
    if (!s || !s->initialized || !task_id) return 0;
    edos_task_t *task = find_task(s, task_id);
    if (!task || task->state != EDOS_TASK_BLOCKED) return 0;
    task->state = EDOS_TASK_READY;
    return 1;
}
