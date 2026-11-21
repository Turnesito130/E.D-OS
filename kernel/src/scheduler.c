#include "scheduler.h"
void edos_scheduler_init(edos_scheduler_t *s) {
    if (!s) return;
    for (unsigned int i=0;i<EDOS_SCHEDULER_MAX_TASKS;++i) {
        s->tasks[i].id=0; s->tasks[i].state=EDOS_TASK_UNUSED;
        s->tasks[i].entry=(void (*)(void))0; s->tasks[i].stack=(void*)0;
    }
    s->next_id=1; s->cursor=0; s->initialized=1;
}
ed_u64 edos_task_create(edos_scheduler_t *s, void (*entry)(void), void *stack) {
    if (!s || !s->initialized || !entry) return 0;
    for (unsigned int i=0;i<EDOS_SCHEDULER_MAX_TASKS;++i) {
        edos_task_t *t=&s->tasks[i];
        if (t->state==EDOS_TASK_UNUSED || t->state==EDOS_TASK_EXITED) {
            ed_u64 id=s->next_id++;
            if (!id) id=s->next_id++;
            t->id=id; t->entry=entry; t->stack=stack; t->state=EDOS_TASK_READY;
            return id;
        }
    }
    return 0;
}
int edos_task_set_state(edos_scheduler_t *s, ed_u64 id, edos_task_state_t state) {
    if (!s || !s->initialized || !id || state<EDOS_TASK_READY || state>EDOS_TASK_EXITED) return 0;
    for (unsigned int i=0;i<EDOS_SCHEDULER_MAX_TASKS;++i) {
        if (s->tasks[i].id==id && s->tasks[i].state!=EDOS_TASK_UNUSED) {
            s->tasks[i].state=state; return 1;
        }
    }
    return 0;
}
edos_task_t *edos_scheduler_pick(edos_scheduler_t *s) {
    if (!s || !s->initialized) return (edos_task_t*)0;
    for (unsigned int n=0;n<EDOS_SCHEDULER_MAX_TASKS;++n) {
        unsigned int i=(unsigned int)((s->cursor+n)%EDOS_SCHEDULER_MAX_TASKS);
        if (s->tasks[i].state==EDOS_TASK_READY || s->tasks[i].state==EDOS_TASK_RUNNING) {
            s->cursor=(i+1U)%EDOS_SCHEDULER_MAX_TASKS;
            return &s->tasks[i];
        }
    }
    return (edos_task_t*)0;
}
