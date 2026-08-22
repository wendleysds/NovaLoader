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

#ifndef _MALLOC_H
#define _MALLOC_H

#include <stddef.h>

void malloc_init(void);

// destructive, will free all memory allocated by malloc
// use with caution
void malloc_reserve_range(size_t start, size_t size);
void malloc_free_range(size_t start, size_t size);

int malloc_range_reserved(size_t start, size_t size);

void* malloc(size_t size);
void free(void* ptr);

#endif