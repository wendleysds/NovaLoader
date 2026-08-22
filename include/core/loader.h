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

#ifndef _LOADER_H
#define _LOADER_H

#include <core/fs.h>
#include <core/platform.h>
#include <def/linker.h>
#include <stddef.h>

struct loader_ops {
    int (*probe)(FILE *file);
    int (*load)(FILE *file, void **out_ctx, char *args[]);
    int (*load_initrd)(void *ctx, FILE *initrd_file, char *args[]);
    void (*unload)(void *ctx);
    int (*boot)(void *ctx); // noreturn if success
};

struct loader {
    const char *name;
    const struct loader_ops *ops;
};

#define REGISTER_LOADER(symbol_name, _name, _ops) \
    static const struct loader loader_##symbol_name \
    __attribute__((used, section(".loaders"))) = { \
        .name = _name, \
        .ops = _ops \
    }

int load_file_segment(FILE *file, size_t offset, void *dest, size_t size);

#endif
