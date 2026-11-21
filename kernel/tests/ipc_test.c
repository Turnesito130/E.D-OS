#include <assert.h>
#include <stdio.h>
#include "ipc.h"
#include "context.h"

static edos_scheduler_t ipc_scheduler;
static edos_ipc_endpoint_t ipc_endpoint;
static edos_cpu_context_t ipc_main_context;
static ed_u64 receiver_id, sender_id;
static unsigned char receiver_stack[4096] __attribute__((aligned(16)));
static unsigned char sender_stack[4096] __attribute__((aligned(16)));
static volatile int ipc_stage;
static void ipc_return_trap(void) { for (;;) {} }

static void ipc_receiver_task(void) {
    edos_ipc_message_t message;
    ipc_stage = 1;
    assert(edos_ipc_receive_wait(&ipc_endpoint, &ipc_scheduler, receiver_id, &message));
    assert(message.sender == sender_id && message.type == 9 && message.length == 5);
    assert(message.payload[0] == 'h' && message.payload[4] == 'o');
    ipc_stage = 4;
    edos_context_switch(&ipc_scheduler.tasks[0].context, &ipc_main_context);
    for (;;) {}
}
static void ipc_sender_task(void) {
    assert(ipc_stage == 1);
    assert(edos_ipc_send_to(&ipc_endpoint, &ipc_scheduler, sender_id, 9, "hello", 5));
    assert(ipc_scheduler.tasks[0].state == EDOS_TASK_READY);
    ipc_stage = 3;
    assert(edos_scheduler_switch_to(&ipc_scheduler, sender_id, receiver_id));
    for (;;) {}
}
int main(void) {
    edos_ipc_queue_t q;
    edos_ipc_message_t in = {0}, out = {0};
    edos_ipc_init(&q);
    assert(!edos_ipc_receive(&q, &out));
    in.sender=7; in.type=2; in.length=3;
    in.payload[0]='E'; in.payload[1]='D'; in.payload[2]='!';
    assert(edos_ipc_send(&q,&in));
    assert(edos_ipc_receive(&q,&out));
    assert(out.sender==7 && out.type==2 && out.length==3 && out.payload[1]=='D');
    for(unsigned int i=0;i<EDOS_IPC_QUEUE_CAPACITY;++i) assert(edos_ipc_send(&q,&in));
    assert(!edos_ipc_send(&q,&in));
    assert(edos_ipc_receive(&q,&out));
    assert(edos_ipc_send(&q,&in));
    in.length=EDOS_IPC_PAYLOAD_MAX+1U;
    assert(!edos_ipc_send(&q,&in));
    edos_ipc_queue_t corrupt={.head=EDOS_IPC_QUEUE_CAPACITY,.tail=0,.count=0};
    assert(!edos_ipc_send(&corrupt,&in));
    corrupt.head=0; corrupt.tail=0; corrupt.count=EDOS_IPC_QUEUE_CAPACITY+1U;
    assert(!edos_ipc_receive(&corrupt,&out));

    edos_scheduler_init(&ipc_scheduler);
    receiver_id = edos_task_create(&ipc_scheduler, ipc_receiver_task, receiver_stack);
    sender_id = edos_task_create(&ipc_scheduler, ipc_sender_task, sender_stack);
    assert(receiver_id && sender_id);
    assert(edos_ipc_endpoint_init(&ipc_endpoint, receiver_id));
    assert(!edos_ipc_endpoint_init(&ipc_endpoint, 0));
    assert(edos_task_prepare_context(&ipc_scheduler, receiver_id, receiver_stack, sizeof(receiver_stack), ipc_return_trap));
    assert(edos_task_prepare_context(&ipc_scheduler, sender_id, sender_stack, sizeof(sender_stack), ipc_return_trap));
    assert(edos_task_set_state(&ipc_scheduler, receiver_id, EDOS_TASK_RUNNING));
    edos_context_switch(&ipc_main_context, &ipc_scheduler.tasks[0].context);
    assert(ipc_stage == 4);
    assert(ipc_scheduler.current_id == receiver_id);
    assert(ipc_scheduler.tasks[0].state == EDOS_TASK_RUNNING);
    assert(ipc_scheduler.tasks[1].state == EDOS_TASK_READY);
    assert(edos_scheduler_validate(&ipc_scheduler));
    puts("IPC_TESTS_OK");
    return 0;
}
