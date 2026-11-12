#ifndef EDOS_MEMORY_H
#define EDOS_MEMORY_H

typedef unsigned long long ed_u64;

/* Bootstrap physical-page allocator. Returns an HHDM virtual pointer to a
 * zeroed 4 KiB page, or 0 when no safe usable page remains. */
int memory_init(void);
void *pmm_alloc_page(void);
ed_u64 memory_usable_bytes(void);
ed_u64 pmm_pages_allocated(void);
ed_u64 memory_hhdm_offset(void);

#endif
