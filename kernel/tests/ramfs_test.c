#include <assert.h>
#include <stdio.h>
#include "ramfs.h"
int main(void) {
 edos_ramfs_t fs; char out[8]={0}; const char data[]="E.D-OS";
 edos_ramfs_init(&fs);
 assert(edos_ramfs_create(&fs,"hello.txt"));
 assert(!edos_ramfs_create(&fs,"hello.txt"));
 assert(!edos_ramfs_create(&fs,"../escape"));
 assert(edos_ramfs_write(&fs,"hello.txt",data,6));
 assert(edos_ramfs_read(&fs,"hello.txt",out,sizeof(out))==6);
 assert(out[0]=='E' && out[5]=='S');
 assert(edos_ramfs_read(&fs,"hello.txt",out,3)==3);
 assert(!edos_ramfs_write(&fs,"hello.txt",data,EDOS_RAMFS_FILE_CAPACITY+1U));
 assert(edos_ramfs_delete(&fs,"hello.txt"));
 assert(edos_ramfs_read(&fs,"hello.txt",out,sizeof(out))==0);
 puts("RAMFS_TESTS_OK"); return 0;
}
