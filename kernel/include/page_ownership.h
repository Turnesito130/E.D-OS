#ifndef EDOS_PAGE_OWNERSHIP_H
#define EDOS_PAGE_OWNERSHIP_H

#include "memory.h"

#define EDOS_PAGE_OWNERSHIP_EMPTY 0ULL
#define EDOS_PAGE_OWNERSHIP_TOMBSTONE (~(ed_u64)0)

typedef struct {
    ed_u64 *slots;
    ed_u64 capacity;
    ed_u64 count;
    int initialized;
} edos_page_ownership_t;

/* Caller supplies a power-of-two table; no allocator dependency. */
int edos_page_ownership_init(edos_page_ownership_t *table, ed_u64 *slots,
                             ed_u64 capacity);
int edos_page_ownership_insert(edos_page_ownership_t *table, ed_u64 page);
int edos_page_ownership_remove(edos_page_ownership_t *table, ed_u64 page);
int edos_page_ownership_contains(const edos_page_ownership_t *table, ed_u64 page);
int edos_page_ownership_validate(const edos_page_ownership_t *table);

#endif
