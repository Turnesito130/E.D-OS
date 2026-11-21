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
 assert(edos_ramfs_create(&fs,"hello.txt"));
 assert(edos_ramfs_read(&fs,"hello.txt",out,sizeof(out))==0);
 /* Truncation followed by a sparse extension must not reveal old bytes. */
 const char secret[]="SECRET";
 assert(edos_ramfs_write(&fs,"hello.txt",secret,6));
 assert(edos_ramfs_write(&fs,"hello.txt",0,0));
 const char marker_byte='X';
 assert(edos_ramfs_write_at(&fs,"hello.txt",3,&marker_byte,1));
 char scrubbed[4]={1,1,1,1};
 assert(edos_ramfs_read(&fs,"hello.txt",scrubbed,sizeof(scrubbed))==4);
 assert(scrubbed[0]==0 && scrubbed[1]==0 && scrubbed[2]==0 && scrubbed[3]=='X');
 /* Rewriting a shorter payload clears the full former tail. */
 assert(edos_ramfs_write(&fs,"hello.txt",secret,6));
 assert(edos_ramfs_write(&fs,"hello.txt","OK",2));
 assert(edos_ramfs_write_at(&fs,"hello.txt",5,&marker_byte,1));
 char gap[6]={1,1,1,1,1,1};
 assert(edos_ramfs_read(&fs,"hello.txt",gap,sizeof(gap))==6);
 assert(gap[0]=='O' && gap[1]=='K' && gap[2]==0 && gap[3]==0 && gap[4]==0 && gap[5]=='X');
 assert(!edos_ramfs_create(&fs,""));
 puts("RAMFS_TESTS_OK"); return 0;
}
