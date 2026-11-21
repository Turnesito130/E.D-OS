#ifndef EDOS_IPC_H
#define EDOS_IPC_H
#include "memory.h"
#include "scheduler.h"
#define EDOS_IPC_QUEUE_CAPACITY 16U
#define EDOS_IPC_PAYLOAD_MAX 64U
typedef struct { ed_u64 sender; ed_u64 type; ed_u64 length; unsigned char payload[EDOS_IPC_PAYLOAD_MAX]; } edos_ipc_message_t;
typedef struct { edos_ipc_message_t messages[EDOS_IPC_QUEUE_CAPACITY]; unsigned int head,tail,count; } edos_ipc_queue_t;
typedef struct { ed_u64 owner_task_id; edos_ipc_queue_t queue; int initialized; } edos_ipc_endpoint_t;
void edos_ipc_init(edos_ipc_queue_t *queue);
int edos_ipc_send(edos_ipc_queue_t *queue,const edos_ipc_message_t *message);
int edos_ipc_receive(edos_ipc_queue_t *queue,edos_ipc_message_t *message);
int edos_ipc_endpoint_init(edos_ipc_endpoint_t *endpoint, ed_u64 owner_task_id);
int edos_ipc_send_to(edos_ipc_endpoint_t *endpoint, edos_scheduler_t *scheduler,
                     ed_u64 sender_id, ed_u64 type, const void *payload, ed_u64 length);
int edos_ipc_receive_wait(edos_ipc_endpoint_t *endpoint, edos_scheduler_t *scheduler,
                          ed_u64 receiver_id, edos_ipc_message_t *message);
#endif
