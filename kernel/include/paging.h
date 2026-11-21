#ifndef EDOS_PAGING_H
#define EDOS_PAGING_H
#include "memory.h"

#define EDOS_PAGING_INVALID (~(ed_u64)0)
#define EDOS_PAGE_PRESENT 0x001ULL
#define EDOS_PAGE_WRITE   0x002ULL
#define EDOS_PAGE_USER    0x004ULL
#define EDOS_PAGE_PWT     0x008ULL
#define EDOS_PAGE_PCD     0x010ULL
#define EDOS_PAGE_GLOBAL  0x100ULL
#define EDOS_PAGE_NX      (1ULL << 63)
#define EDOS_PAGE_ADDRESS_MASK 0x000ffffffffff000ULL

typedef ed_u64 (*edos_paging_alloc_fn)(void *opaque);
typedef ed_u64 *(*edos_paging_phys_to_virt_fn)(ed_u64 physical, void *opaque);

typedef struct {
    ed_u64 root_physical;
    edos_paging_alloc_fn alloc_page;
    edos_paging_phys_to_virt_fn phys_to_virt;
    void *opaque;
    int initialized;
} edos_paging_space_t;

int edos_paging_init(edos_paging_space_t *space,
                     edos_paging_alloc_fn alloc_page,
                     edos_paging_phys_to_virt_fn phys_to_virt,
                     void *opaque);
int edos_paging_adopt(edos_paging_space_t *space, ed_u64 root_physical,
                      edos_paging_alloc_fn alloc_page,
                      edos_paging_phys_to_virt_fn phys_to_virt, void *opaque);
int edos_paging_find_unused_pml4(const edos_paging_space_t *space, ed_u64 *virtual_out);
int edos_paging_map_4k(edos_paging_space_t *space, ed_u64 virtual_address,
                       ed_u64 physical_address, ed_u64 flags);
int edos_paging_unmap_4k(edos_paging_space_t *space, ed_u64 virtual_address,
                         ed_u64 *physical_out);
int edos_paging_translate(const edos_paging_space_t *space,
                          ed_u64 virtual_address, ed_u64 *physical_out,
                          ed_u64 *flags_out);
#endif
