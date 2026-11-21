#ifndef EDOS_IPC_H
#define EDOS_IPC_H
#include "memory.h"
#define EDOS_IPC_QUEUE_CAPACITY 16U
#define EDOS_IPC_PAYLOAD_MAX 64U
typedef struct { ed_u64 sender; ed_u64 type; ed_u64 length; unsigned char payload[EDOS_IPC_PAYLOAD_MAX]; } edos_ipc_message_t;
typedef struct { edos_ipc_message_t messages[EDOS_IPC_QUEUE_CAPACITY]; unsigned int head,tail,count; } edos_ipc_queue_t;
void edos_ipc_init(edos_ipc_queue_t *queue);
int edos_ipc_send(edos_ipc_queue_t *queue,const edos_ipc_message_t *message);
int edos_ipc_receive(edos_ipc_queue_t *queue,edos_ipc_message_t *message);
#endif
