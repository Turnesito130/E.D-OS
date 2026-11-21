#ifndef EDOS_BUDDY_H
#define EDOS_BUDDY_H

#include "memory.h"

#define BUDDY_MAX_ORDER 20U
#define BUDDY_NONE (~(ed_u64)0)

typedef struct {
    ed_u64 next;
    ed_u64 prev;
    unsigned int order;
    unsigned char state;
} buddy_page_t;

typedef struct {
    buddy_page_t *pages;
    ed_u64 page_count;
    ed_u64 free_pages;
    ed_u64 free_heads[BUDDY_MAX_ORDER + 1U];
    unsigned int max_order;
    int initialized;
} buddy_allocator_t;

/* Metadata is supplied by the caller; this allocator never allocates memory itself. */
int buddy_init(buddy_allocator_t *allocator, buddy_page_t *metadata,
               ed_u64 page_count);
int buddy_add_range(buddy_allocator_t *allocator, ed_u64 first_page,
                    ed_u64 page_count);
ed_u64 buddy_alloc(buddy_allocator_t *allocator, unsigned int order);
int buddy_free(buddy_allocator_t *allocator, ed_u64 first_page,
               unsigned int order);
ed_u64 buddy_free_page_count(const buddy_allocator_t *allocator);
int buddy_validate(const buddy_allocator_t *allocator);

#endif
