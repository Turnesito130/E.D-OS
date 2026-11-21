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
