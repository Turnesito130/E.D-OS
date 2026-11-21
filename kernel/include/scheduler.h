#ifndef EDOS_SCHEDULER_H
#define EDOS_SCHEDULER_H
#include "memory.h"
#include "context.h"
#define EDOS_SCHEDULER_MAX_TASKS 32U
typedef enum { EDOS_TASK_UNUSED=0, EDOS_TASK_READY=1, EDOS_TASK_RUNNING=2, EDOS_TASK_BLOCKED=3, EDOS_TASK_EXITED=4 } edos_task_state_t;
typedef struct { ed_u64 id; edos_task_state_t state; void (*entry)(void); void *stack; edos_cpu_context_t context; int context_ready; } edos_task_t;
typedef struct { edos_task_t tasks[EDOS_SCHEDULER_MAX_TASKS]; ed_u64 next_id; ed_u64 cursor; ed_u64 current_id; int initialized; } edos_scheduler_t;
void edos_scheduler_init(edos_scheduler_t *scheduler);
ed_u64 edos_task_create(edos_scheduler_t *scheduler, void (*entry)(void), void *stack);
int edos_task_set_state(edos_scheduler_t *scheduler, ed_u64 id, edos_task_state_t state);
edos_task_t *edos_scheduler_pick(edos_scheduler_t *scheduler);
edos_task_t *edos_scheduler_schedule(edos_scheduler_t *scheduler);
int edos_scheduler_validate(const edos_scheduler_t *scheduler);
int edos_scheduler_switch_to(edos_scheduler_t *scheduler, ed_u64 from_id, ed_u64 to_id);
int edos_task_prepare_context(edos_scheduler_t *scheduler, ed_u64 id,
                              void *stack_base, ed_u64 stack_size,
                              void (*return_trap)(void));
#endif
