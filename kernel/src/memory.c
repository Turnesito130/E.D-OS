/* E.D-OS bootstrap memory discovery and monotonic physical page allocator.
 * Limine owns the response structures; this module never modifies or frees
 * bootloader-reclaimable memory. */
#include "memory.h"

#define PAGE_SIZE 4096ULL
#define MIN_SAFE_PHYS 0x100000ULL
#define MEMMAP_USABLE 0ULL

typedef struct {
    ed_u64 base;
    ed_u64 length;
    ed_u64 type;
} limine_memmap_entry;

typedef struct {
    ed_u64 revision;
    ed_u64 entry_count;
    limine_memmap_entry **entries;
} limine_memmap_response;

typedef struct {
    ed_u64 revision;
    ed_u64 offset;
} limine_hhdm_response;

typedef struct {
    ed_u64 id[4];
    ed_u64 revision;
    limine_memmap_response *response;
} limine_memmap_request;

typedef struct {
    ed_u64 id[4];
    ed_u64 revision;
    limine_hhdm_response *response;
} limine_hhdm_request;

__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile ed_u64 memmap_request[6] = {
    0xc7b1dd30df4c8b88ULL, 0x0a82e883a194f07bULL,
    0x67cf3d9d378a806fULL, 0xe304acdfc50c3c62ULL,
    0, 0
};
__attribute__((used, section(".limine_requests"), aligned(8)))
static volatile ed_u64 hhdm_request[6] = {
    0xc7b1dd30df4c8b88ULL, 0x0a82e883a194f07bULL,
    0x48dcf1cb8ad2b852ULL, 0x63984e959a98244bULL,
    0, 0
};

/* The protocol writes the response pointer into word 5 of each request. */
static limine_memmap_response *get_memmap_response(void) {
    return (limine_memmap_response *)(unsigned long long)memmap_request[5];
}
static limine_hhdm_response *get_hhdm_response(void) {
    return (limine_hhdm_response *)(unsigned long long)hhdm_request[5];
}

static limine_memmap_response *map;
static ed_u64 hhdm;
static ed_u64 usable_bytes;
static ed_u64 allocated_pages;
static ed_u64 region_index;
static ed_u64 region_cursor;
static int initialized;

static int add_overflows(ed_u64 a, ed_u64 b) {
    return b > (~0ULL - a);
}

int memory_init(void) {
    map = get_memmap_response();
    limine_hhdm_response *direct = get_hhdm_response();
    if (map == 0 || direct == 0 || map->entries == 0 ||
        map->entry_count == 0 || map->entry_count > 4096) {
        return 0;
    }

    hhdm = direct->offset;
    usable_bytes = 0;
    allocated_pages = 0;
    region_index = 0;
    region_cursor = 0;

    for (ed_u64 i = 0; i < map->entry_count; ++i) {
        limine_memmap_entry *entry = map->entries[i];
        if (entry == 0 || entry->type != MEMMAP_USABLE ||
            entry->length == 0 || add_overflows(entry->base, entry->length)) {
            continue;
        }
        if (add_overflows(usable_bytes, entry->length)) {
            usable_bytes = ~0ULL;
        } else {
            usable_bytes += entry->length;
        }
    }
    initialized = usable_bytes != 0;
    return initialized;
}

void *pmm_alloc_page(void) {
    if (!initialized || map == 0) return (void *)0;

    while (region_index < map->entry_count) {
        limine_memmap_entry *entry = map->entries[region_index];
        if (entry == 0 || entry->type != MEMMAP_USABLE ||
            entry->length == 0 || add_overflows(entry->base, entry->length)) {
            ++region_index;
            region_cursor = 0;
            continue;
        }

        ed_u64 start = entry->base;
        if (start < MIN_SAFE_PHYS) start = MIN_SAFE_PHYS;
        if (start > (~0ULL - (PAGE_SIZE - 1))) {
            ++region_index;
            region_cursor = 0;
            continue;
        }
        start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        ed_u64 end = (entry->base + entry->length) & ~(PAGE_SIZE - 1);
        ed_u64 candidate = region_cursor > start ? region_cursor : start;

        if (candidate <= end && end - candidate >= PAGE_SIZE) {
            region_cursor = candidate + PAGE_SIZE;
            ++allocated_pages;
            ed_u64 virtual_address = hhdm + candidate;
            if (virtual_address < hhdm) return (void *)0;
            volatile ed_u64 *page = (volatile ed_u64 *)(unsigned long long)virtual_address;
            for (ed_u64 i = 0; i < PAGE_SIZE / sizeof(ed_u64); ++i) {
                page[i] = 0;
            }
            return (void *)(unsigned long long)virtual_address;
        }

        ++region_index;
        region_cursor = 0;
    }
    return (void *)0;
}

ed_u64 memory_usable_bytes(void) { return usable_bytes; }
ed_u64 pmm_pages_allocated(void) { return allocated_pages; }
ed_u64 memory_hhdm_offset(void) { return hhdm; }
