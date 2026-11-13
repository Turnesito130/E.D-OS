#ifndef EDOS_MEMORY_H
#define EDOS_MEMORY_H
typedef unsigned long long ed_u64;
int memory_init(void);
void *pmm_alloc_page(void);
int pmm_free_page(void *page);
ed_u64 memory_usable_bytes(void);
ed_u64 pmm_pages_allocated(void);
ed_u64 pmm_pages_free(void);
ed_u64 memory_hhdm_offset(void);
#endif
