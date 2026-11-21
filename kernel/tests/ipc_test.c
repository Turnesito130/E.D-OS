#include <assert.h>
#include <stdio.h>
#include "ipc.h"
int main(void) {
 edos_ipc_queue_t q; edos_ipc_message_t in={0},out={0};
 edos_ipc_init(&q); assert(!edos_ipc_receive(&q,&out));
 in.sender=7; in.type=2; in.length=3; in.payload[0]='E';in.payload[1]='D';in.payload[2]='!';
 assert(edos_ipc_send(&q,&in)); assert(edos_ipc_receive(&q,&out));
 assert(out.sender==7 && out.type==2 && out.length==3 && out.payload[1]=='D');
 for(unsigned int i=0;i<EDOS_IPC_QUEUE_CAPACITY;++i) assert(edos_ipc_send(&q,&in));
 assert(!edos_ipc_send(&q,&in));
 assert(edos_ipc_receive(&q,&out)); assert(edos_ipc_send(&q,&in));
 in.length=EDOS_IPC_PAYLOAD_MAX+1U; assert(!edos_ipc_send(&q,&in));
 edos_ipc_queue_t corrupt={.head=EDOS_IPC_QUEUE_CAPACITY,.tail=0,.count=0};
 assert(!edos_ipc_send(&corrupt,&in));
 corrupt.head=0; corrupt.tail=0; corrupt.count=EDOS_IPC_QUEUE_CAPACITY+1U;
 assert(!edos_ipc_receive(&corrupt,&out));
 puts("IPC_TESTS_OK"); return 0;
}
