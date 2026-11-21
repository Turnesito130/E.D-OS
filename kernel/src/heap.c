#include "heap.h"

#define HEAP_PAGE_SIZE 4096ULL
#define HEAP_MAGIC 0x45444f5348454150ULL
#define HEAP_SLOT_OFFSET 64ULL
#define HEAP_BITMAP_WORDS 4U

typedef unsigned int ed_u32;

typedef struct heap_slab {
    struct heap_slab *next;
    ed_u64 magic;
    ed_u64 bitmap[HEAP_BITMAP_WORDS];
    ed_u32 class_index;
    ed_u32 slot_count;
    ed_u32 free_count;
    ed_u32 reserved;
} heap_slab_t;

static const ed_u32 class_sizes[EDOS_HEAP_CLASS_COUNT] = {
    16U, 32U, 64U, 128U, 256U, 512U, 1024U, 2048U, 4032U
};

static void zero_bytes(void *pointer, ed_u64 length) {
    unsigned char *bytes = (unsigned char *)pointer;
    for (ed_u64 i = 0; i < length; ++i) bytes[i] = 0;
}

static ed_u32 slots_for_class(ed_u32 class_index) {
    return (ed_u32)((HEAP_PAGE_SIZE - HEAP_SLOT_OFFSET) /
                    class_sizes[class_index]);
}

static int bit_is_set(const heap_slab_t *slab, ed_u32 index) {
    return (slab->bitmap[index / 64U] & (1ULL << (index % 64U))) != 0;
}

static void bit_set(heap_slab_t *slab, ed_u32 index) {
    slab->bitmap[index / 64U] |= 1ULL << (index % 64U);
}

static void bit_clear(heap_slab_t *slab, ed_u32 index) {
    slab->bitmap[index / 64U] &= ~(1ULL << (index % 64U));
}

static heap_slab_t *create_slab(edos_heap_t *heap, ed_u32 class_index) {
    heap_slab_t *slab = (heap_slab_t *)heap->allocate_page(heap->context);
    if (!slab) return (heap_slab_t *)0;
    zero_bytes(slab, HEAP_PAGE_SIZE);
    slab->next = (heap_slab_t *)heap->slabs[class_index];
    slab->magic = HEAP_MAGIC;
    slab->class_index = class_index;
    slab->slot_count = slots_for_class(class_index);
    slab->free_count = slab->slot_count;
    heap->slabs[class_index] = slab;
    ++heap->page_counts[class_index];
    return slab;
}

int edos_heap_init(edos_heap_t *heap, edos_heap_page_alloc_fn allocate_page,
                   edos_heap_page_free_fn free_page, void *context) {
    if (!heap || !allocate_page || !free_page) return 0;
    for (ed_u32 i = 0; i < EDOS_HEAP_CLASS_COUNT; ++i) {
        heap->slabs[i] = (void *)0;
        heap->page_counts[i] = 0;
    }
    heap->allocate_page = allocate_page;
    heap->free_page = free_page;
    heap->context = context;
    heap->initialized = 1;
    return 1;
}

void *edos_heap_alloc(edos_heap_t *heap, ed_u64 size) {
    if (!heap || !heap->initialized || !size || size > class_sizes[EDOS_HEAP_CLASS_COUNT - 1U])
        return (void *)0;
    ed_u32 class_index = 0;
    while (class_index < EDOS_HEAP_CLASS_COUNT && size > class_sizes[class_index])
        ++class_index;
    if (class_index == EDOS_HEAP_CLASS_COUNT) return (void *)0;

    heap_slab_t *slab = (heap_slab_t *)heap->slabs[class_index];
    while (slab && !slab->free_count) slab = slab->next;
    if (!slab) slab = create_slab(heap, class_index);
    if (!slab || slab->magic != HEAP_MAGIC) return (void *)0;

    for (ed_u32 index = 0; index < slab->slot_count; ++index) {
        if (bit_is_set(slab, index)) continue;
        bit_set(slab, index);
        --slab->free_count;
        unsigned char *slot = (unsigned char *)slab + HEAP_SLOT_OFFSET +
                              (ed_u64)index * class_sizes[class_index];
        zero_bytes(slot, class_sizes[class_index]);
        return slot;
    }
    return (void *)0;
}

int edos_heap_free(edos_heap_t *heap, void *pointer) {
    if (!heap || !heap->initialized || !pointer) return 0;
    ed_u64 address = (ed_u64)(unsigned long long)pointer;

    for (ed_u32 class_index = 0; class_index < EDOS_HEAP_CLASS_COUNT; ++class_index) {
        heap_slab_t *previous = (heap_slab_t *)0;
        heap_slab_t *slab = (heap_slab_t *)heap->slabs[class_index];
        ed_u64 traversed = 0;
        while (slab && traversed++ < heap->page_counts[class_index]) {
            ed_u64 base = (ed_u64)(unsigned long long)slab;
            ed_u64 start = base + HEAP_SLOT_OFFSET;
            ed_u64 end = start + (ed_u64)slab->slot_count * class_sizes[class_index];
            if (slab->magic == HEAP_MAGIC && slab->class_index == class_index &&
                address >= start && address < end) {
                ed_u64 offset = address - start;
                if (offset % class_sizes[class_index]) return 0;
                ed_u32 index = (ed_u32)(offset / class_sizes[class_index]);
                if (index >= slab->slot_count || !bit_is_set(slab, index)) return 0;
                zero_bytes(pointer, class_sizes[class_index]);
                bit_clear(slab, index);
                ++slab->free_count;

                /* Keep one empty slab per class; return additional empty pages to PMM. */
                if (slab->free_count == slab->slot_count &&
                    heap->page_counts[class_index] > 1U) {
                    if (previous) previous->next = slab->next;
                    else heap->slabs[class_index] = slab->next;
                    if (heap->free_page(heap->context, slab))
                        --heap->page_counts[class_index];
                    else {
                        if (previous) previous->next = slab;
                        else heap->slabs[class_index] = slab;
                    }
                }
                return 1;
            }
            previous = slab;
            slab = slab->next;
        }
        if (slab) return 0;
    }
    return 0;
}

int edos_heap_validate(const edos_heap_t *heap) {
    if (!heap || !heap->initialized || !heap->allocate_page || !heap->free_page)
        return 0;
    for (ed_u32 class_index = 0; class_index < EDOS_HEAP_CLASS_COUNT; ++class_index) {
        ed_u64 pages = 0;
        const heap_slab_t *slab = (const heap_slab_t *)heap->slabs[class_index];
        while (slab) {
            if (++pages > heap->page_counts[class_index] ||
                slab->magic != HEAP_MAGIC || slab->class_index != class_index ||
                slab->slot_count != slots_for_class(class_index) ||
                slab->free_count > slab->slot_count) return 0;
            ed_u32 used = 0;
            for (ed_u32 i = 0; i < HEAP_BITMAP_WORDS * 64U; ++i) {
                int set = bit_is_set(slab, i);
                if (i < slab->slot_count) used += set ? 1U : 0U;
                else if (set) return 0;
            }
            if (used + slab->free_count != slab->slot_count) return 0;
            slab = slab->next;
        }
        if (pages != heap->page_counts[class_index]) return 0;
    }
    return 1;
}
