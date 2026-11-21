#include <assert.h>
#include <stdio.h>
#include "page_ownership.h"

int main(void) {
    edos_page_ownership_t table;
    ed_u64 slots[8];
    assert(!edos_page_ownership_init(&table, slots, 7));
    assert(edos_page_ownership_init(&table, slots, 8));
    assert(edos_page_ownership_validate(&table));

    const ed_u64 a = 0x1000ULL;
    const ed_u64 b = 0x9000ULL;
    const ed_u64 c = 0x11000ULL;
    assert(edos_page_ownership_insert(&table, a));
    assert(edos_page_ownership_insert(&table, b));
    assert(edos_page_ownership_insert(&table, c));
    assert(!edos_page_ownership_insert(&table, a));
    assert(!edos_page_ownership_insert(&table, 0x1001ULL));
    assert(edos_page_ownership_contains(&table, a));
    assert(edos_page_ownership_contains(&table, b));
    assert(edos_page_ownership_validate(&table));

    assert(edos_page_ownership_remove(&table, b));
    assert(!edos_page_ownership_contains(&table, b));
    assert(!edos_page_ownership_remove(&table, b));
    assert(edos_page_ownership_insert(&table, 0x19000ULL));
    assert(edos_page_ownership_contains(&table, c));
    assert(edos_page_ownership_validate(&table));

    for (ed_u64 i = 0; i < 8; ++i) {
        ed_u64 page = (i + 32U) * 4096ULL;
        if (!edos_page_ownership_contains(&table, page))
            assert(edos_page_ownership_insert(&table, page));
    }
    assert(!edos_page_ownership_insert(&table, 0x80000ULL));
    assert(edos_page_ownership_validate(&table));
    puts("PAGE_OWNERSHIP_TESTS_OK");
    return 0;
}
