#include "ipc.h"
static int queue_valid(const edos_ipc_queue_t *q) {
    return q && q->head < EDOS_IPC_QUEUE_CAPACITY &&
           q->tail < EDOS_IPC_QUEUE_CAPACITY && q->count <= EDOS_IPC_QUEUE_CAPACITY;
}
void edos_ipc_init(edos_ipc_queue_t *q) { if(q){q->head=0;q->tail=0;q->count=0;} }
int edos_ipc_send(edos_ipc_queue_t *q,const edos_ipc_message_t *m) {
    if(!queue_valid(q) || !m || m->length>EDOS_IPC_PAYLOAD_MAX || q->count>=EDOS_IPC_QUEUE_CAPACITY) return 0;
    q->messages[q->tail]=*m;
    q->tail=(q->tail+1U)%EDOS_IPC_QUEUE_CAPACITY;
    ++q->count; return 1;
}
int edos_ipc_receive(edos_ipc_queue_t *q,edos_ipc_message_t *m) {
    if(!queue_valid(q) || !m || !q->count) return 0;
    *m=q->messages[q->head];
    q->head=(q->head+1U)%EDOS_IPC_QUEUE_CAPACITY;
    --q->count; return 1;
}

int edos_ipc_endpoint_init(edos_ipc_endpoint_t *endpoint, ed_u64 owner_task_id) {
    if (!endpoint || !owner_task_id) return 0;
    endpoint->owner_task_id = owner_task_id;
    endpoint->initialized = 0;
    edos_ipc_init(&endpoint->queue);
    endpoint->initialized = 1;
    return 1;
}

int edos_ipc_send_to(edos_ipc_endpoint_t *endpoint, edos_scheduler_t *scheduler,
                     ed_u64 sender_id, ed_u64 type, const void *payload, ed_u64 length) {
    if (!endpoint || !endpoint->initialized || !scheduler || !scheduler->initialized ||
        !sender_id || scheduler->current_id != sender_id ||
        length > EDOS_IPC_PAYLOAD_MAX || (!payload && length)) return 0;
    edos_ipc_message_t message;
    message.sender = sender_id;
    message.type = type;
    message.length = length;
    const unsigned char *src = (const unsigned char *)payload;
    for (ed_u64 i = 0; i < length; ++i) message.payload[i] = src[i];
    if (!edos_ipc_send(&endpoint->queue, &message)) return 0;
    (void)edos_scheduler_wake(scheduler, endpoint->owner_task_id);
    return 1;
}

int edos_ipc_receive_wait(edos_ipc_endpoint_t *endpoint, edos_scheduler_t *scheduler,
                          ed_u64 receiver_id, edos_ipc_message_t *message) {
    if (!endpoint || !endpoint->initialized || !scheduler || !scheduler->initialized ||
        !receiver_id || receiver_id != endpoint->owner_task_id ||
        scheduler->current_id != receiver_id || !message) return 0;
    if (edos_ipc_receive(&endpoint->queue, message)) return 1;
    edos_task_t *next = edos_scheduler_pick(scheduler);
    if (!next || !next->context_ready) return 0;
    if (!edos_scheduler_block_current(scheduler, receiver_id, next->id)) return 0;
    return edos_ipc_receive(&endpoint->queue, message);
}
