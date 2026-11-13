#include "memory.h"
#define PAGE_SIZE 4096ULL
#define MIN_SAFE_PHYS 0x100000ULL
#define MEMMAP_USABLE 0ULL
typedef struct { ed_u64 base, length, type; } limine_memmap_entry;
typedef struct { ed_u64 revision, entry_count; limine_memmap_entry **entries; } limine_memmap_response;
typedef struct { ed_u64 revision, offset; } limine_hhdm_response;
__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile ed_u64 memmap_request[6] = {
0xc7b1dd30df4c8b88ULL,0x0a82e883a194f07bULL,0x67cf3d9d378a806fULL,0xe304acdfc50c3c62ULL,0,0};
__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile ed_u64 hhdm_request[6] = {
0xc7b1dd30df4c8b88ULL,0x0a82e883a194f07bULL,0x48dcf1cb8ad2b852ULL,0x63984e959a98244bULL,0,0};
static limine_memmap_response *get_memmap_response(void) {
return (limine_memmap_response *)(unsigned long long)memmap_request[5];}
static limine_hhdm_response *get_hhdm_response(void) {
return (limine_hhdm_response *)(unsigned long long)hhdm_request[5];}
static limine_memmap_response *map;
static ed_u64 hhdm, usable_bytes, allocated_pages, free_pages, region_index, region_cursor;
static void *free_list;
static int initialized;
static int add_overflows(ed_u64 a, ed_u64 b) { return b > (~0ULL-a); }
static int physical_page_is_usable(ed_u64 p) {
if ((p & (PAGE_SIZE-1)) || p<MIN_SAFE_PHYS || add_overflows(p,PAGE_SIZE)) return 0;
for(ed_u64 i=0;i<map->entry_count;++i) {
limine_memmap_entry *e=map->entries[i];
if(!e || e->type!=MEMMAP_USABLE || !e->length || add_overflows(e->base,e->length)) continue;
if(p>=e->base && p+PAGE_SIZE<=e->base+e->length) return 1;
} return 0;
}
int memory_init(void) {
map=get_memmap_response(); limine_hhdm_response *d=get_hhdm_response();
if(!map || !d || !map->entries || !map->entry_count || map->entry_count>4096) return 0;
hhdm=d->offset; usable_bytes=allocated_pages=free_pages=region_index=region_cursor=0;
free_list=(void*)0;
for(ed_u64 i=0;i<map->entry_count;++i) {
limine_memmap_entry *e=map->entries[i];
if(!e || e->type!=MEMMAP_USABLE || !e->length || add_overflows(e->base,e->length)) continue;
if(add_overflows(usable_bytes,e->length)) usable_bytes=~0ULL; else usable_bytes+=e->length;
}
initialized=usable_bytes!=0; return initialized;
}
void *pmm_alloc_page(void) {
if(!initialized || !map) return (void*)0;
ed_u64 physical;
if(free_list) {
ed_u64 va=(ed_u64)(unsigned long long)free_list;
if(va<hhdm || free_pages==0) return (void*)0;
ed_u64 *link=(ed_u64*)(unsigned long long)va;
physical=va-hhdm; free_list=(void*)(unsigned long long)*link; --free_pages;
} else {
for(;;) {
if(region_index>=map->entry_count) return (void*)0;
limine_memmap_entry *e=map->entries[region_index];
if(!e || e->type!=MEMMAP_USABLE || !e->length || add_overflows(e->base,e->length)) {++region_index;region_cursor=0;continue;}
ed_u64 start=e->base<MIN_SAFE_PHYS?MIN_SAFE_PHYS:e->base;
if(start>(~0ULL-(PAGE_SIZE-1))) {++region_index;region_cursor=0;continue;}
start=(start+PAGE_SIZE-1)&~(PAGE_SIZE-1);
ed_u64 end=(e->base+e->length)&~(PAGE_SIZE-1);
ed_u64 candidate=region_cursor>start?region_cursor:start;
if(candidate<=end && end-candidate>=PAGE_SIZE) {physical=candidate;region_cursor=candidate+PAGE_SIZE;break;}
++region_index;region_cursor=0;
}}
if(add_overflows(hhdm,physical)) return (void*)0;
ed_u64 va=hhdm+physical;
volatile ed_u64 *words=(volatile ed_u64*)(unsigned long long)va;
for(ed_u64 i=0;i<PAGE_SIZE/sizeof(ed_u64);++i) words[i]=0;
++allocated_pages; return (void*)(unsigned long long)va;
}
int pmm_free_page(void *page) {
if(!initialized || !page || !allocated_pages) return 0;
ed_u64 va=(ed_u64)(unsigned long long)page;
if(va<hhdm) return 0;
ed_u64 physical=va-hhdm;
if(!physical_page_is_usable(physical)) return 0;
/* The caller contract requires a page returned by pmm_alloc_page. */
void *node=free_list;
for(ed_u64 seen=0;node && seen<=free_pages;++seen) {
if(node==page) return 0;
node=(void*)(unsigned long long)*(ed_u64*)node;
}
if(node) return 0;
*(ed_u64*)page=(ed_u64)(unsigned long long)free_list;
free_list=page; ++free_pages; --allocated_pages; return 1;
}
ed_u64 memory_usable_bytes(void){return usable_bytes;}
ed_u64 pmm_pages_allocated(void){return allocated_pages;}
ed_u64 pmm_pages_free(void){return free_pages;}
ed_u64 memory_hhdm_offset(void){return hhdm;}
