#include "ramfs.h"
static int valid_name(const char *name) {
 if(!name || !name[0]) return 0;
 ed_u64 n=0; while(name[n] && n<=EDOS_RAMFS_NAME_MAX) { if(name[n]=='/' || name[n]=='\\') return 0; ++n; }
 return n>0 && n<=EDOS_RAMFS_NAME_MAX;
}
static edos_ramfs_file_t *find_file(edos_ramfs_t *fs,const char *name) {
 if(!fs || !fs->initialized || !valid_name(name)) return (edos_ramfs_file_t*)0;
 for(unsigned int i=0;i<EDOS_RAMFS_MAX_FILES;++i) {
  edos_ramfs_file_t *f=&fs->files[i]; unsigned int j=0;
  if(!f->used) continue;
  while(f->name[j] && name[j] && f->name[j]==name[j]) ++j;
  if(!f->name[j] && !name[j]) return f;
 }
 return (edos_ramfs_file_t*)0;
}
void edos_ramfs_init(edos_ramfs_t *fs) {
 if(!fs) return;
 for(unsigned int i=0;i<EDOS_RAMFS_MAX_FILES;++i) { fs->files[i].used=0; fs->files[i].size=0; fs->files[i].name[0]=0; }
 fs->initialized=1;
}
int edos_ramfs_create(edos_ramfs_t *fs,const char *name) {
 if(!fs || !fs->initialized || !valid_name(name) || find_file(fs,name)) return 0;
 for(unsigned int i=0;i<EDOS_RAMFS_MAX_FILES;++i) if(!fs->files[i].used) {
  edos_ramfs_file_t *f=&fs->files[i]; unsigned int j=0;
  while(name[j] && j<=EDOS_RAMFS_NAME_MAX) { f->name[j]=name[j]; ++j; }
  f->name[j]=0; f->size=0;
  for(ed_u64 k=0;k<EDOS_RAMFS_FILE_CAPACITY;++k) f->data[k]=0;
  f->used=1; return 1;
 }
 return 0;
}
int edos_ramfs_write(edos_ramfs_t *fs,const char *name,const void *data,ed_u64 length) {
 edos_ramfs_file_t *f=find_file(fs,name);
 if(!f || (!data && length) || length>EDOS_RAMFS_FILE_CAPACITY) return 0;
 const unsigned char *src=(const unsigned char*)data;
 for(ed_u64 i=0;i<length;++i) f->data[i]=src[i];
 f->size=length; return 1;
}
ed_u64 edos_ramfs_read(edos_ramfs_t *fs,const char *name,void *buffer,ed_u64 capacity) {
 edos_ramfs_file_t *f=find_file(fs,name);
 if(!f || (!buffer && capacity)) return 0;
 ed_u64 count=f->size<capacity?f->size:capacity;
 unsigned char *dst=(unsigned char*)buffer;
 for(ed_u64 i=0;i<count;++i) dst[i]=f->data[i];
 return count;
}
int edos_ramfs_delete(edos_ramfs_t *fs,const char *name) {
 edos_ramfs_file_t *f=find_file(fs,name); if(!f) return 0;
 for(ed_u64 i=0;i<EDOS_RAMFS_FILE_CAPACITY;++i) f->data[i]=0;
 f->used=0; f->size=0; f->name[0]=0; return 1;
}

ed_u64 edos_ramfs_read_at(edos_ramfs_t *fs,const char *name,ed_u64 offset,void *buffer,ed_u64 capacity) {
 edos_ramfs_file_t *f=find_file(fs,name);
 if(!f || (!buffer && capacity) || offset>f->size) return 0;
 ed_u64 remaining=f->size-offset;
 ed_u64 count=remaining<capacity?remaining:capacity;
 unsigned char *dst=(unsigned char*)buffer;
 for(ed_u64 i=0;i<count;++i) dst[i]=f->data[offset+i];
 return count;
}
int edos_ramfs_write_at(edos_ramfs_t *fs,const char *name,ed_u64 offset,const void *data,ed_u64 length) {
 edos_ramfs_file_t *f=find_file(fs,name);
 if(!f || (!data && length) || offset>EDOS_RAMFS_FILE_CAPACITY ||
    length>EDOS_RAMFS_FILE_CAPACITY-offset) return 0;
 const unsigned char *src=(const unsigned char*)data;
 for(ed_u64 i=0;i<length;++i) f->data[offset+i]=src[i];
 if(offset+length>f->size) f->size=offset+length;
 return 1;
}
