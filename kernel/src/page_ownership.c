#include "page_ownership.h"

#define PAGE_SIZE 4096ULL
#define HASH_MULTIPLIER 11400714819323198485ULL

static ed_u64 hash_page(const edos_page_ownership_t *table, ed_u64 page) {
    return ((page / PAGE_SIZE) * HASH_MULTIPLIER) & (table->capacity - 1U);
}

int edos_page_ownership_init(edos_page_ownership_t *table, ed_u64 *slots,
                             ed_u64 capacity) {
    if (!table || !slots || capacity < 2U || (capacity & (capacity - 1U)))
        return 0;
    table->slots = slots;
    table->capacity = capacity;
    table->count = 0;
    table->initialized = 0;
    for (ed_u64 i = 0; i < capacity; ++i)
        slots[i] = EDOS_PAGE_OWNERSHIP_EMPTY;
    table->initialized = 1;
    return 1;
}

int edos_page_ownership_contains(const edos_page_ownership_t *table, ed_u64 page) {
    if (!table || !table->initialized || !page ||
        (page & (PAGE_SIZE - 1U)) || page == EDOS_PAGE_OWNERSHIP_TOMBSTONE)
        return 0;
    ed_u64 index = hash_page(table, page);
    for (ed_u64 seen = 0; seen < table->capacity; ++seen) {
        ed_u64 value = table->slots[index];
        if (value == EDOS_PAGE_OWNERSHIP_EMPTY) return 0;
        if (value == page) return 1;
        index = (index + 1U) & (table->capacity - 1U);
    }
    return 0;
}

int edos_page_ownership_insert(edos_page_ownership_t *table, ed_u64 page) {
    if (!table || !table->initialized || !page ||
        (page & (PAGE_SIZE - 1U)) || page == EDOS_PAGE_OWNERSHIP_TOMBSTONE ||
        table->count >= table->capacity || edos_page_ownership_contains(table, page))
        return 0;
    ed_u64 index = hash_page(table, page);
    ed_u64 tombstone = EDOS_PAGE_OWNERSHIP_EMPTY;
    for (ed_u64 seen = 0; seen < table->capacity; ++seen) {
        ed_u64 value = table->slots[index];
        if (value == EDOS_PAGE_OWNERSHIP_TOMBSTONE && tombstone == EDOS_PAGE_OWNERSHIP_EMPTY)
            tombstone = index;
        if (value == EDOS_PAGE_OWNERSHIP_EMPTY) {
            table->slots[tombstone == EDOS_PAGE_OWNERSHIP_EMPTY ? index : tombstone] = page;
            ++table->count;
            return 1;
        }
        index = (index + 1U) & (table->capacity - 1U);
    }
    if (tombstone != EDOS_PAGE_OWNERSHIP_EMPTY) {
        table->slots[tombstone] = page;
        ++table->count;
        return 1;
    }
    return 0;
}

int edos_page_ownership_remove(edos_page_ownership_t *table, ed_u64 page) {
    if (!table || !table->initialized || !page ||
        (page & (PAGE_SIZE - 1U)))
        return 0;
    ed_u64 index = hash_page(table, page);
    for (ed_u64 seen = 0; seen < table->capacity; ++seen) {
        ed_u64 value = table->slots[index];
        if (value == EDOS_PAGE_OWNERSHIP_EMPTY) return 0;
        if (value == page) {
            table->slots[index] = EDOS_PAGE_OWNERSHIP_TOMBSTONE;
            --table->count;
            return 1;
        }
        index = (index + 1U) & (table->capacity - 1U);
    }
    return 0;
}

int edos_page_ownership_validate(const edos_page_ownership_t *table) {
    if (!table || !table->initialized || !table->slots ||
        table->capacity < 2U || (table->capacity & (table->capacity - 1U)) ||
        table->count > table->capacity)
        return 0;
    ed_u64 count = 0;
    for (ed_u64 i = 0; i < table->capacity; ++i) {
        ed_u64 page = table->slots[i];
        if (page == EDOS_PAGE_OWNERSHIP_EMPTY ||
            page == EDOS_PAGE_OWNERSHIP_TOMBSTONE) continue;
        if (!page || (page & (PAGE_SIZE - 1U)) ||
            !edos_page_ownership_contains(table, page)) return 0;
        ++count;
    }
    return count == table->count;
}
