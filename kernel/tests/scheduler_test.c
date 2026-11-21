#include <assert.h>
#include <stdio.h>
#include "scheduler.h"
static void task_a(void){} static void task_b(void){}
int main(void) {
 edos_scheduler_t s; edos_scheduler_init(&s);
 ed_u64 a=edos_task_create(&s,task_a,(void*)1), b=edos_task_create(&s,task_b,(void*)2);
 assert(a && b && a!=b);
 assert(edos_scheduler_pick(&s)->id==a);
 assert(edos_scheduler_pick(&s)->id==b);
 assert(edos_task_set_state(&s,a,EDOS_TASK_BLOCKED));
 assert(edos_scheduler_pick(&s)->id==b);
 assert(edos_task_set_state(&s,a,EDOS_TASK_READY));
 assert(edos_task_set_state(&s,b,EDOS_TASK_EXITED));
 assert(edos_task_set_state(&s,a,EDOS_TASK_EXITED));
 assert(edos_scheduler_pick(&s)==0);
 assert(edos_task_create(&s,task_a,0)!=0);
 puts("SCHEDULER_TESTS_OK"); return 0;
}
