#include "paging.h"

#define PAGE_ENTRIES 512U
#define PAGE_SIZE 4096ULL
#define PAGE_HUGE 0x080ULL
#define ALLOWED_FLAGS (EDOS_PAGE_PRESENT | EDOS_PAGE_WRITE | EDOS_PAGE_USER | \
                       EDOS_PAGE_PWT | EDOS_PAGE_PCD | EDOS_PAGE_GLOBAL | EDOS_PAGE_NX)

static int canonical_address(ed_u64 address) {
    ed_u64 upper = address >> 48;
    return ((address >> 47) & 1ULL) ? upper == 0xffffULL : upper == 0;
}

static ed_u64 *table_pointer(const edos_paging_space_t *space, ed_u64 entry) {
    if (!(entry & EDOS_PAGE_PRESENT) || (entry & PAGE_HUGE))
        return (ed_u64 *)0;
    return space->phys_to_virt(entry & EDOS_PAGE_ADDRESS_MASK, space->opaque);
}

static void clear_table(ed_u64 *table) {
    for (unsigned int i = 0; i < PAGE_ENTRIES; ++i) table[i] = 0;
}

int edos_paging_init(edos_paging_space_t *space,
                     edos_paging_alloc_fn alloc_page,
                     edos_paging_phys_to_virt_fn phys_to_virt,
                     void *opaque) {
    if (!space || !alloc_page || !phys_to_virt) return 0;
    space->initialized = 0;
    space->alloc_page = alloc_page;
    space->phys_to_virt = phys_to_virt;
    space->opaque = opaque;
    ed_u64 root = alloc_page(opaque);
    if (root == EDOS_PAGING_INVALID || (root & (PAGE_SIZE - 1ULL)) ||
        (root & ~EDOS_PAGE_ADDRESS_MASK)) return 0;
    ed_u64 *root_table = phys_to_virt(root, opaque);
    if (!root_table) return 0;
    clear_table(root_table);
    space->root_physical = root;
    space->initialized = 1;
    return 1;
}

static ed_u64 *ensure_child(edos_paging_space_t *space, ed_u64 *parent,
                            unsigned int index, int user) {
    ed_u64 entry = parent[index];
    if (entry & EDOS_PAGE_PRESENT) {
        if (entry & PAGE_HUGE) return (ed_u64 *)0;
        if (user) parent[index] |= EDOS_PAGE_USER;
        return table_pointer(space, parent[index]);
    }
    ed_u64 physical = space->alloc_page(space->opaque);
    if (physical == EDOS_PAGING_INVALID || (physical & (PAGE_SIZE - 1ULL)) ||
        (physical & ~EDOS_PAGE_ADDRESS_MASK)) return (ed_u64 *)0;
    ed_u64 *child = space->phys_to_virt(physical, space->opaque);
    if (!child) return (ed_u64 *)0;
    clear_table(child);
    parent[index] = physical | EDOS_PAGE_PRESENT | EDOS_PAGE_WRITE |
                    (user ? EDOS_PAGE_USER : 0ULL);
    return child;
}

int edos_paging_map_4k(edos_paging_space_t *space, ed_u64 va,
                       ed_u64 pa, ed_u64 flags) {
    if (!space || !space->initialized || !canonical_address(va) ||
        (va & (PAGE_SIZE - 1ULL)) || (pa & (PAGE_SIZE - 1ULL)) ||
        (pa & ~EDOS_PAGE_ADDRESS_MASK) || !(flags & EDOS_PAGE_PRESENT) ||
        (flags & ~ALLOWED_FLAGS)) return 0;
    int user = (flags & EDOS_PAGE_USER) != 0;
    ed_u64 *table = space->phys_to_virt(space->root_physical, space->opaque);
    if (!table) return 0;
    const unsigned int indexes[3] = {
        (unsigned int)((va >> 39) & 0x1ffULL),
        (unsigned int)((va >> 30) & 0x1ffULL),
        (unsigned int)((va >> 21) & 0x1ffULL)
    };
    for (unsigned int level = 0; level < 3; ++level) {
        table = ensure_child(space, table, indexes[level], user);
        if (!table) return 0;
    }
    unsigned int leaf = (unsigned int)((va >> 12) & 0x1ffULL);
    if (table[leaf] & EDOS_PAGE_PRESENT) return 0;
    table[leaf] = pa | flags;
    return 1;
}

int edos_paging_translate(const edos_paging_space_t *space,
                          ed_u64 va, ed_u64 *physical_out, ed_u64 *flags_out) {
    if (!space || !space->initialized || !canonical_address(va) ||
        !physical_out) return 0;
    ed_u64 *table = space->phys_to_virt(space->root_physical, space->opaque);
    if (!table) return 0;
    const unsigned int indexes[4] = {
        (unsigned int)((va >> 39) & 0x1ffULL),
        (unsigned int)((va >> 30) & 0x1ffULL),
        (unsigned int)((va >> 21) & 0x1ffULL),
        (unsigned int)((va >> 12) & 0x1ffULL)
    };
    for (unsigned int level = 0; level < 3; ++level) {
        ed_u64 entry = table[indexes[level]];
        if (!(entry & EDOS_PAGE_PRESENT) || (entry & PAGE_HUGE)) return 0;
        table = table_pointer(space, entry);
        if (!table) return 0;
    }
    ed_u64 leaf = table[indexes[3]];
    if (!(leaf & EDOS_PAGE_PRESENT)) return 0;
    *physical_out = (leaf & EDOS_PAGE_ADDRESS_MASK) | (va & (PAGE_SIZE - 1ULL));
    if (flags_out) *flags_out = leaf & ~EDOS_PAGE_ADDRESS_MASK;
    return 1;
}

int edos_paging_unmap_4k(edos_paging_space_t *space, ed_u64 va,
                         ed_u64 *physical_out) {
    if (!space || !space->initialized || !canonical_address(va) ||
        (va & (PAGE_SIZE - 1ULL))) return 0;
    ed_u64 *table = space->phys_to_virt(space->root_physical, space->opaque);
    if (!table) return 0;
    const unsigned int indexes[4] = {
        (unsigned int)((va >> 39) & 0x1ffULL),
        (unsigned int)((va >> 30) & 0x1ffULL),
        (unsigned int)((va >> 21) & 0x1ffULL),
        (unsigned int)((va >> 12) & 0x1ffULL)
    };
    for (unsigned int level = 0; level < 3; ++level) {
        ed_u64 entry = table[indexes[level]];
        if (!(entry & EDOS_PAGE_PRESENT) || (entry & PAGE_HUGE)) return 0;
        table = table_pointer(space, entry);
        if (!table) return 0;
    }
    ed_u64 *leaf = &table[indexes[3]];
    if (!(*leaf & EDOS_PAGE_PRESENT)) return 0;
    if (physical_out) *physical_out = *leaf & EDOS_PAGE_ADDRESS_MASK;
    *leaf = 0;
    return 1;
}
