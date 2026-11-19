#include "buddy.h"

#define PAGE_RESERVED 0U
#define PAGE_FREE 1U
#define PAGE_ALLOCATED 2U
#define PAGE_TAIL 3U

static ed_u64 block_size(unsigned int order) {
    return (ed_u64)1U << order;
}

static void list_remove(buddy_allocator_t *a, ed_u64 index) {
    buddy_page_t *p = &a->pages[index];
    ed_u64 head = a->free_heads[p->order];

    if (p->prev != BUDDY_NONE)
        a->pages[p->prev].next = p->next;
    else if (head == index)
        a->free_heads[p->order] = p->next;
    else
        return;

    if (p->next != BUDDY_NONE)
        a->pages[p->next].prev = p->prev;
    p->next = BUDDY_NONE;
    p->prev = BUDDY_NONE;
}

static void list_insert(buddy_allocator_t *a, ed_u64 index,
                        unsigned int order) {
    buddy_page_t *p = &a->pages[index];
    ed_u64 old_head = a->free_heads[order];

    p->order = order;
    p->state = PAGE_FREE;
    p->prev = BUDDY_NONE;
    p->next = old_head;
    if (old_head != BUDDY_NONE)
        a->pages[old_head].prev = index;
    a->free_heads[order] = index;
}

static void mark_block(buddy_allocator_t *a, ed_u64 first,
                       unsigned int order, unsigned char state) {
    ed_u64 count = block_size(order);
    for (ed_u64 i = 0; i < count; ++i) {
        buddy_page_t *p = &a->pages[first + i];
        p->state = (i == 0U) ? state : PAGE_TAIL;
        p->order = (i == 0U) ? order : 0U;
        p->next = BUDDY_NONE;
        p->prev = BUDDY_NONE;
    }
}

int buddy_init(buddy_allocator_t *a, buddy_page_t *metadata,
               ed_u64 page_count) {
    if (!a || !metadata || !page_count)
        return 0;

    a->pages = metadata;
    a->page_count = page_count;
    a->free_pages = 0;
    a->max_order = 0;
    a->initialized = 0;

    while (a->max_order < BUDDY_MAX_ORDER &&
           block_size(a->max_order + 1U) <= page_count)
        ++a->max_order;

    for (unsigned int i = 0; i <= BUDDY_MAX_ORDER; ++i)
        a->free_heads[i] = BUDDY_NONE;

    for (ed_u64 i = 0; i < page_count; ++i) {
        metadata[i].next = BUDDY_NONE;
        metadata[i].prev = BUDDY_NONE;
        metadata[i].order = 0;
        metadata[i].state = PAGE_RESERVED;
    }

    a->initialized = 1;
    return 1;
}

int buddy_add_range(buddy_allocator_t *a, ed_u64 first_page,
                    ed_u64 page_count) {
    if (!a || !a->initialized || !page_count ||
        first_page >= a->page_count ||
        page_count > a->page_count - first_page)
        return 0;

    ed_u64 end = first_page + page_count;
    /* Reject overlap before mutating any allocator state. */
    for (ed_u64 i = first_page; i < end; ++i) {
        if (a->pages[i].state != PAGE_RESERVED)
            return 0;
    }

    ed_u64 cursor = first_page;
    while (cursor < end) {
        unsigned int order = a->max_order;
        while (order > 0U) {
            ed_u64 size = block_size(order);
            if ((cursor & (size - 1U)) == 0U && size <= end - cursor)
                break;
            --order;
        }

        mark_block(a, cursor, order, PAGE_FREE);
        list_insert(a, cursor, order);
        a->free_pages += block_size(order);
        cursor += block_size(order);
    }
    return 1;
}

ed_u64 buddy_alloc(buddy_allocator_t *a, unsigned int order) {
    if (!a || !a->initialized || order > a->max_order ||
        order > BUDDY_MAX_ORDER)
        return BUDDY_NONE;

    unsigned int found = order;
    while (found <= a->max_order &&
           a->free_heads[found] == BUDDY_NONE)
        ++found;
    if (found > a->max_order)
        return BUDDY_NONE;

    ed_u64 index = a->free_heads[found];
    list_remove(a, index);

    while (found > order) {
        --found;
        ed_u64 right = index + block_size(found);
        mark_block(a, index, found, PAGE_FREE);
        mark_block(a, right, found, PAGE_FREE);
        list_insert(a, right, found);
    }

    mark_block(a, index, order, PAGE_ALLOCATED);
    a->free_pages -= block_size(order);
    return index;
}

int buddy_free(buddy_allocator_t *a, ed_u64 first_page,
               unsigned int order) {
    if (!a || !a->initialized || order > a->max_order ||
        order > BUDDY_MAX_ORDER || first_page >= a->page_count)
        return 0;

    ed_u64 count = block_size(order);
    if ((first_page & (count - 1U)) != 0U ||
        count > a->page_count - first_page)
        return 0;

    buddy_page_t *head = &a->pages[first_page];
    if (head->state != PAGE_ALLOCATED || head->order != order)
        return 0;

    for (ed_u64 i = 1; i < count; ++i) {
        if (a->pages[first_page + i].state != PAGE_TAIL)
            return 0;
    }

    ed_u64 index = first_page;
    unsigned int merged_order = order;
    mark_block(a, index, merged_order, PAGE_FREE);

    while (merged_order < a->max_order) {
        ed_u64 buddy = index ^ block_size(merged_order);
        if (buddy >= a->page_count ||
            a->pages[buddy].state != PAGE_FREE ||
            a->pages[buddy].order != merged_order)
            break;

        list_remove(a, buddy);
        if (buddy < index)
            index = buddy;
        ++merged_order;
        mark_block(a, index, merged_order, PAGE_FREE);
    }

    list_insert(a, index, merged_order);
    a->free_pages += count;
    return 1;
}

ed_u64 buddy_free_page_count(const buddy_allocator_t *a) {
    return (a && a->initialized) ? a->free_pages : 0U;
}
