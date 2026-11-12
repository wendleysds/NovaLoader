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

#define BLOCK_SIZE 2048 

static const size_t slab_sizes[] = { 8, 16, 32, 64, 128, 256, 512, 1024 };
#define NUM_SLAB_SIZES ARRAY_SIZE(slab_sizes)

static struct slab_cache slab_caches[NUM_SLAB_SIZES];

static uintptr_t arena_cur = 0;

static inline uintptr_t get_block_base(uintptr_t addr) {
    return ALIGN_DOWN(addr, BLOCK_SIZE);
}

static uintptr_t alloc_block() {
    uintptr_t ret = ALIGN_UP(arena_cur, BLOCK_SIZE);
    arena_cur = ret + BLOCK_SIZE;
    return ret;
}

static int get_cache_index(size_t size) {
    for (int i = 0; i < NUM_SLAB_SIZES; i++) {
        if (size <= slab_sizes[i]) return i;
    }
    return -1;
}

static struct slab *slab_create(struct slab_cache *cache) {
    uintptr_t addr = alloc_block();
    if (!addr) return NULL;

    struct slab *slab = (struct slab*)addr;
    memset(slab, 0, sizeof(*slab));

    uintptr_t obj_start = ALIGN_UP(addr + sizeof(struct slab), CPU_ALIGNMENT);
    slab->start = (void*)obj_start;
    
    slab->total_objects = (BLOCK_SIZE - (obj_start - addr)) / cache->object_size;
    slab->free_count = slab->total_objects;
    slab->free_list = NULL;

    uintptr_t current_obj = (uintptr_t)slab->start;
    for (uint32_t i = 0; i < slab->total_objects; i++) {
        *(void**)current_obj = slab->free_list;
        slab->free_list = (void*)current_obj;
        current_obj += cache->object_size;
    }

    INIT_LIST_HEAD(&slab->list);
    return slab;
}

void malloc_init(void) {
    for (int i = 0; i < NUM_SLAB_SIZES; i++) {
        slab_caches[i].object_size = slab_sizes[i];
        INIT_LIST_HEAD(&slab_caches[i].slabs);
    }

    arena_cur = ALIGN_UP((size_t)__bss_end, sizeof(int)); 
}

void *malloc(size_t size) {
    if (size == 0 || size > slab_sizes[NUM_SLAB_SIZES - 1]) return NULL;

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
    {
        void *obj = slab->free_list;
        slab->free_list = *(void**)obj;
        slab->free_count--;
        return obj;
    }
}

void free(void *ptr) {
    if (!ptr) return;

    uintptr_t base = get_block_base((uintptr_t)ptr);
    struct slab *slab = (struct slab*)base;

    *(void**)ptr = slab->free_list;
    slab->free_list = ptr;
    slab->free_count++;
}
