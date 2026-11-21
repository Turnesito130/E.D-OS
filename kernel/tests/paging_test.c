#include <assert.h>
#include <stdio.h>
#include "paging.h"

#define TEST_PAGES 32U
static ed_u64 pages[TEST_PAGES][512];
static unsigned int allocated;
static ed_u64 alloc_page(void *opaque) {
    (void)opaque;
    if (allocated >= TEST_PAGES) return EDOS_PAGING_INVALID;
    return (ed_u64)(allocated++) * 4096ULL;
}
static ed_u64 *phys_to_virt(ed_u64 physical, void *opaque) {
    (void)opaque;
    if ((physical & 4095ULL) || physical / 4096ULL >= TEST_PAGES) return 0;
    return pages[physical / 4096ULL];
}
int main(void) {
    edos_paging_space_t space;
    ed_u64 translated = 0, flags = 0, unmapped = 0;
    assert(edos_paging_init(&space, alloc_page, phys_to_virt, 0));
    const ed_u64 va = 0xffff800000001000ULL;
    assert(edos_paging_map_4k(&space, va, 0x9000, EDOS_PAGE_PRESENT | EDOS_PAGE_WRITE));
    assert(edos_paging_translate(&space, va + 0x123, &translated, &flags));
    assert(translated == 0x9123);
    assert((flags & (EDOS_PAGE_PRESENT | EDOS_PAGE_WRITE)) ==
           (EDOS_PAGE_PRESENT | EDOS_PAGE_WRITE));
    assert(!edos_paging_map_4k(&space, va, 0x3000, EDOS_PAGE_PRESENT));
    assert(!edos_paging_map_4k(&space, 0x0000800000000000ULL, 0x3000, EDOS_PAGE_PRESENT));
    assert(!edos_paging_map_4k(&space, va + 1, 0x3000, EDOS_PAGE_PRESENT));
    assert(!edos_paging_map_4k(&space, va + 0x1000, 0x3000, 0));
    assert(edos_paging_unmap_4k(&space, va, &unmapped));
    assert(unmapped == 0x9000);
    assert(!edos_paging_translate(&space, va, &translated, 0));
    assert(!edos_paging_unmap_4k(&space, va, 0));
    edos_paging_space_t adopted;
    assert(edos_paging_adopt(&adopted, space.root_physical, alloc_page, phys_to_virt, 0));
    ed_u64 unused_va = 0;
    assert(edos_paging_find_unused_pml4(&adopted, &unused_va));
    assert(((unused_va >> 39) & 0x1ffULL) == 257ULL);
    puts("PAGING_TESTS_OK");
    return 0;
}
