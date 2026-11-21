#include <assert.h>
#include <stdio.h>
#include "buddy.h"

int main(void) {
    buddy_allocator_t allocator;
    buddy_page_t metadata[64];

    assert(buddy_init(&allocator, metadata, 64));
    assert(buddy_add_range(&allocator, 0, 64));
    assert(buddy_free_page_count(&allocator) == 64);
    assert(buddy_validate(&allocator));
    assert(!buddy_add_range(&allocator, 0, 4));
    assert(buddy_free_page_count(&allocator) == 64);

    ed_u64 a = buddy_alloc(&allocator, 2);
    ed_u64 b = buddy_alloc(&allocator, 1);
    assert(a == 0);
    assert(b == 4);
    assert(buddy_free_page_count(&allocator) == 58);
    assert(buddy_validate(&allocator));
    assert(!buddy_free(&allocator, a, 1));
    assert(buddy_free(&allocator, a, 2));
    assert(!buddy_free(&allocator, a, 2));
    assert(buddy_free(&allocator, b, 1));
    assert(buddy_free_page_count(&allocator) == 64);

    ed_u64 whole = buddy_alloc(&allocator, 6);
    assert(whole == 0);
    assert(buddy_alloc(&allocator, 0) == BUDDY_NONE);
    assert(!buddy_free(&allocator, whole, 5));
    assert(buddy_free(&allocator, whole, 6));
    assert(buddy_free_page_count(&allocator) == 64);
    assert(buddy_validate(&allocator));

    buddy_allocator_t partial;
    buddy_page_t partial_metadata[32];
    assert(buddy_init(&partial, partial_metadata, 32));
    assert(buddy_add_range(&partial, 3, 20));
    assert(buddy_free_page_count(&partial) == 20);
    assert(buddy_validate(&partial));
    ed_u64 p = buddy_alloc(&partial, 2);
    assert(p != BUDDY_NONE);
    assert((p & 3U) == 0U);
    assert(buddy_free(&partial, p, 2));
    assert(buddy_free_page_count(&partial) == 20);
    assert(buddy_validate(&partial));
    ed_u64 saved_head = partial.free_heads[partial.pages[partial.free_heads[0]].order];
    (void)saved_head;

    puts("BUDDY_TESTS_OK");
    return 0;
}
