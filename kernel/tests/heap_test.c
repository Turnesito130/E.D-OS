#include <assert.h>
#include <stdio.h>
#include "heap.h"

#define TEST_PAGES 32U
static unsigned char page_pool[TEST_PAGES][4096] __attribute__((aligned(4096)));
static unsigned char page_used[TEST_PAGES];

static void *test_alloc_page(void *context) {
    (void)context;
    for (unsigned int i = 0; i < TEST_PAGES; ++i) {
        if (page_used[i]) continue;
        page_used[i] = 1;
        for (unsigned int j = 0; j < 4096U; ++j) page_pool[i][j] = 0;
        return page_pool[i];
    }
    return 0;
}

static int test_free_page(void *context, void *page) {
    (void)context;
    for (unsigned int i = 0; i < TEST_PAGES; ++i) {
        if (page_pool[i] != page || !page_used[i]) continue;
        page_used[i] = 0;
        return 1;
    }
    return 0;
}

int main(void) {
    edos_heap_t heap;
    assert(edos_heap_init(&heap, test_alloc_page, test_free_page, 0));
    assert(edos_heap_validate(&heap));
    assert(edos_heap_alloc(&heap, 0) == 0);
    assert(edos_heap_alloc(&heap, 4033) == 0);

    const ed_u64 sizes[] = {1, 16, 17, 64, 129, 255, 700, 1025, 2048, 2049, 4032};
    void *objects[sizeof(sizes) / sizeof(sizes[0])];
    for (unsigned int i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        objects[i] = edos_heap_alloc(&heap, sizes[i]);
        assert(objects[i]);
        assert(((unsigned long long)objects[i] & 15ULL) == 0);
        const unsigned char *bytes = (const unsigned char *)objects[i];
        for (ed_u64 j = 0; j < sizes[i]; ++j) assert(bytes[j] == 0);
    }
    assert(edos_heap_validate(&heap));

    unsigned char *first = (unsigned char *)objects[0];
    first[0] = 0xa5U;
    assert(edos_heap_free(&heap, first));
    assert(!edos_heap_free(&heap, first));
    assert(!edos_heap_free(&heap, first + 1));
    void *reused = edos_heap_alloc(&heap, 1);
    assert(reused == first);
    assert(*(unsigned char *)reused == 0);
    assert(edos_heap_validate(&heap));

    void *small[260];
    for (unsigned int i = 0; i < 260U; ++i) {
        small[i] = edos_heap_alloc(&heap, 16);
        assert(small[i]);
    }
    assert(edos_heap_validate(&heap));
    for (unsigned int i = 0; i < 260U; ++i) assert(edos_heap_free(&heap, small[i]));
    assert(edos_heap_validate(&heap));
    for (unsigned int i = 1; i < sizeof(objects) / sizeof(objects[0]); ++i)
        assert(edos_heap_free(&heap, objects[i]));
    assert(edos_heap_validate(&heap));
    puts("KERNEL_HEAP_TESTS_OK");
    return 0;
}
