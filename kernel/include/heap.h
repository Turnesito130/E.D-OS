#ifndef EDOS_HEAP_H
#define EDOS_HEAP_H

#include "memory.h"

#define EDOS_HEAP_CLASS_COUNT 9U

typedef void *(*edos_heap_page_alloc_fn)(void *context);
typedef int (*edos_heap_page_free_fn)(void *context, void *page);

typedef struct {
    void *slabs[EDOS_HEAP_CLASS_COUNT];
    ed_u64 page_counts[EDOS_HEAP_CLASS_COUNT];
    edos_heap_page_alloc_fn allocate_page;
    edos_heap_page_free_fn free_page;
    void *context;
    int initialized;
} edos_heap_t;

int edos_heap_init(edos_heap_t *heap, edos_heap_page_alloc_fn allocate_page,
                   edos_heap_page_free_fn free_page, void *context);
void *edos_heap_alloc(edos_heap_t *heap, ed_u64 size);
int edos_heap_free(edos_heap_t *heap, void *pointer);
int edos_heap_validate(const edos_heap_t *heap);

#endif
