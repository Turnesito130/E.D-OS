#include "scheduler.h"

static edos_task_t *find_task(edos_scheduler_t *s, ed_u64 id) {
    if (!s || !id) return (edos_task_t *)0;
    for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i)
        if (s->tasks[i].id == id && s->tasks[i].state != EDOS_TASK_UNUSED)
            return &s->tasks[i];
    return (edos_task_t *)0;
}

void edos_scheduler_init(edos_scheduler_t *s) {
    if (!s) return;
    for (unsigned int i = 0; i < EDOS_SCHEDULER_MAX_TASKS; ++i) {
        s->tasks[i].id = 0;
        s->tasks[i].state = EDOS_TASK_UNUSED;
        s->tasks[i].entry = (void (*)(void))0;
        s->tasks[i].stack = (void *)0;
    }
    s->next_id = 1;
    s->cursor = 0;
    s->current_id = 0;
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

/* Updates scheduler bookkeeping; it does not save/restore CPU registers or stacks. */
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
        if (!t->id || !t->entry) return 0;
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
