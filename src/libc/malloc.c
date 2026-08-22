/*
 * SPDX-License-Identifier: MIT
 *
 * NovaLoader - Lightweight, modular, and configurable x86 bootloader for BIOS/MBR
 *
 * Copyright (c) 2026 Wendley Santos
 *
 * This file is part of NovaLoader.
 * See the LICENSE file in the project root for license information.
 */

#include <core/platform.h>
#include <def/linker.h>
#include <string.h>

#if USE_ARENA_ALLOCATOR == 1

static size_t cur;
static size_t end;

void malloc_init(void) {
    cur = ALIGN_UP((size_t)__bss_end, sizeof(int));
    end = ALIGN_DOWN(HEAP_END, sizeof(int));
}

void malloc_reserve_range(size_t start, size_t size){
    // cannot free in arena allocator
}

void malloc_free_range(size_t start, size_t size){
    // cannot reserve in arena allocator
}

int malloc_range_reserved(size_t start, size_t size){
	return start < cur;
}

void *malloc(size_t size) {
    size_t ret = ALIGN_UP(cur, CPU_ALIGNMENT);
    if(ret + size > end) return NULL;
    cur = ret + size;
    return (void*)ret;
}

void free(void *ptr) {
    // cannot free in arena allocator
}

#else

typedef enum {
    BLOCK_FREE  = 0,
    BLOCK_USED  = (1 << 0),
    BLOCK_FIRST = (1 << 1),
    BLOCK_NEXT  = (1 << 2),
    BLOCK_SLAB  = (1 << 3),
} block_flags_t;

struct slab {
    struct list_head list;
    void *start;
    void *free_list;
    uint32_t free_count;
    uint32_t total_objects;
};

struct slab_cache {
    size_t object_size;
    struct list_head slabs;
};

static const size_t slab_sizes[] = SLAB_SIZES;
#define NUM_SLAB_SIZES ARRAY_SIZE(slab_sizes)
#define SLAB_MAX_SIZE (slab_sizes[NUM_SLAB_SIZES - 1])

static struct slab_cache slab_caches[NUM_SLAB_SIZES];

static struct {
    uintptr_t start;
    uintptr_t end;
    uint8_t *metadata;
    size_t count;
    size_t last_hint;
} heap;

static uint8_t metadata[ 
	HEAP_END / BLOCK_SIZE
];

static inline size_t addr_to_idx(uintptr_t addr) {
    return (addr - heap.start) / BLOCK_SIZE;
}

static inline uintptr_t idx_to_addr(size_t idx) {
    return heap.start + (idx *BLOCK_SIZE);
}

static inline int block_is_free(uint8_t flags) {
    return flags == BLOCK_FREE;
}

static inline uintptr_t get_block_base(uintptr_t addr) {
    return ALIGN_DOWN(addr, BLOCK_SIZE);
}

static uintptr_t alloc_blocks(size_t count, uint8_t flags) {
    if (count == 0 || count > heap.count) return 0;

    size_t found = 0;
    size_t start_idx = 0;
    size_t checked = 0;
    size_t i = heap.last_hint;

    while (checked < heap.count) {
        if (block_is_free(heap.metadata[i])) {
            if (found == 0) start_idx = i;
            
            if (++found == count) 
                goto found_space;
        } else {
            found = 0;
        }

        i++;
        checked++;

        if (i == heap.count) {
            i = 0;
            found = 0;
        }
    }

    return 0;

found_space:
    for (size_t j = 0; j < count; j++) {
        size_t idx = start_idx + j;
        heap.metadata[idx] = BLOCK_USED;
        
        if (j == 0)
            heap.metadata[idx] |= BLOCK_FIRST | flags;;
        if (j < count - 1)
            heap.metadata[idx] |= BLOCK_NEXT;
    }

    heap.last_hint = (start_idx + count) % heap.count;
    return idx_to_addr(start_idx);
}

static void free_blocks(uintptr_t start) {
    size_t idx = addr_to_idx(start);

    if (!(heap.metadata[idx] & BLOCK_FIRST)) return;

	heap.last_hint = idx;
    while (idx < heap.count) {
        uint8_t flags = heap.metadata[idx];
        heap.metadata[idx] = BLOCK_FREE;
        
        if (!(flags & BLOCK_NEXT)) break;
        idx++;
    }
}

static void slab_init(void) {
    for (int i = 0; i < NUM_SLAB_SIZES; i++) {
        slab_caches[i].object_size = slab_sizes[i];
        INIT_LIST_HEAD(&slab_caches[i].slabs);
    }
}

static int get_cache_index(size_t size) {
    for (int i = 0; i < NUM_SLAB_SIZES; i++) {
        if (size <= slab_sizes[i]) return i;
    }
    return -1;
}

static struct slab *slab_create(struct slab_cache *cache) {
    uintptr_t addr = alloc_blocks(1, BLOCK_SLAB);
    if (!addr) return NULL;

    struct slab *slab = (struct slab*)addr;
    memset(slab, 0, sizeof(*slab));

    uintptr_t obj_start = addr + sizeof(struct slab);
    slab->start = (void*)obj_start;
    slab->total_objects = (BLOCK_SIZE - sizeof(struct slab)) / cache->object_size;
    slab->free_count = slab->total_objects;

    void *obj = slab->start;
    for (uint32_t i = 0; i < slab->total_objects; i++) {
        *(void**)obj = slab->free_list;
        slab->free_list = obj;
        obj = (void*)((uintptr_t)obj + cache->object_size);
    }

    INIT_LIST_HEAD(&slab->list);
    return slab;
}

static void *slab_alloc(size_t size) {
    if (size == 0) return NULL;
	void *obj = NULL;

    int cache_idx = get_cache_index(size);
    if (cache_idx < 0) return NULL;

    struct slab_cache *cache = &slab_caches[cache_idx];
    struct slab *slab = NULL;

    list_for_each_entry(slab, &cache->slabs, list) {
        if (slab->free_count > 0) goto found;
    }

    slab = slab_create(cache);
    if (!slab) return NULL;
    list_add(&slab->list, &cache->slabs);

found:
    obj = slab->free_list;
    slab->free_list = *(void**)obj;
    slab->free_count--;
    return obj;
}

static void slab_free(void *ptr) {
    if (!ptr) return;

    uintptr_t base = get_block_base((uintptr_t)ptr);
    size_t idx = addr_to_idx(base);

    if (!(heap.metadata[idx] & BLOCK_SLAB)) return;

    struct slab *slab = (struct slab*)base;

    *(void**)ptr = slab->free_list;
    slab->free_list = ptr;
    slab->free_count++;

    if (slab->free_count == slab->total_objects) {
        list_remove(&slab->list);
        free_blocks(base);
    }
}

// public API

void malloc_init(void) {
	heap.start = 0;
	heap.end = HEAP_END;
	heap.metadata = metadata;	
	heap.count = sizeof(metadata);
	heap.last_hint = 0;
	memset(metadata, BLOCK_FREE, heap.count);
	slab_init();

	void *iter = NULL;
	struct mem_info mem;
	while(mmap_iter(&mem, &iter)){
		size_t start = mem.base;
		size_t end = mem.base + mem.size;

		if(start > HEAP_END) start = HEAP_END;
		if(end > HEAP_END) end = HEAP_END;

		if(mem.reserved){
			malloc_reserve_range(start, end - start);
		}
	}

	malloc_reserve_range(0,  ALIGN_UP((size_t)__bss_end, BLOCK_SIZE));
}

void malloc_reserve_range(size_t start, size_t size){
	const size_t end = start + size;

	const uintptr_t aligned_start = ALIGN_UP(start, BLOCK_SIZE);
	const uintptr_t aligned_end = ALIGN_DOWN(end, BLOCK_SIZE);

	const size_t start_idx = addr_to_idx(aligned_start);
	const size_t end_idx = addr_to_idx(aligned_end);

	for (size_t j = start_idx; j < end_idx; j++) {
		heap.metadata[j] = BLOCK_USED;
	}
}

void malloc_free_range(size_t start, size_t size){
	const size_t end = start + size;

	const uintptr_t aligned_start = ALIGN_UP(start, BLOCK_SIZE);
	const uintptr_t aligned_end = ALIGN_DOWN(end, BLOCK_SIZE);

	const size_t start_idx = addr_to_idx(aligned_start);
	const size_t end_idx = addr_to_idx(aligned_end);

	for (size_t j = start_idx; j < end_idx; j++) {
		heap.metadata[j] = BLOCK_FREE;
	}
}

int malloc_range_reserved(size_t start, size_t size){
	const size_t end = start + size;

	const uintptr_t aligned_start = ALIGN_UP(start, BLOCK_SIZE);
	const uintptr_t aligned_end = ALIGN_DOWN(end, BLOCK_SIZE);

	const size_t start_idx = addr_to_idx(aligned_start);
	const size_t end_idx = addr_to_idx(aligned_end);

	for (size_t j = start_idx; j < end_idx; j++) {
		if (heap.metadata[j] != BLOCK_USED) {
			return 0;
		}
	}

	return 1;
}

void *malloc(size_t size) {
    if (size == 0) return NULL;

    if (size > SLAB_MAX_SIZE) {
        const size_t aligned = ALIGN_UP(size, BLOCK_SIZE);
        return (void*)alloc_blocks(aligned / BLOCK_SIZE, 0);
    }

    return slab_alloc(size);
}

void free(void *ptr) {
    if (!ptr) return;

    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t base_addr = get_block_base(addr);
    
    size_t idx = addr_to_idx(base_addr);
    uint8_t flags = heap.metadata[idx];

    if (flags & BLOCK_SLAB) {
        slab_free(ptr);
    } else if (flags & BLOCK_FIRST) {
        if (addr == base_addr) {
            free_blocks(base_addr);
        }
    }
}

#endif
