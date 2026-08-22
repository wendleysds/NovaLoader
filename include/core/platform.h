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

#ifndef _PLATFORM_H
#define _PLATFORM_H

#include <core/disk.h>
#include <def/attributes.h>
#include <def/errno.h>
#include <lib/list.h>
#include <lib/partition.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* =========================================================================
 * 1. HARDWARE ABSTRACTION LAYER (HAL) STRUCTURES
 * ========================================================================= */

struct mem_info {
  uint32_t base;
  uint32_t size;
  uint32_t atype;
  bool reserved;
};

struct platform_terminal {
  int (*init)(void);

  void (*putc)(char c);
  void (*clear)(void);
  void (*set_cursor)(int x, int y);
  void (*get_cursor)(int *x, int *y);
  void (*get_resolution)(int *w, int *h);
};

struct platform_input {
  int (*init)(void);

  int (*get_key)(void);
  int (*poll_key)(void);
};

struct platform_time {
  int (*init)(void);
  uint64_t (*get_ticks)(void);
};

struct platform_system {
  int (*init)(void);

  int (*mmap_iter)(struct mem_info *buff, void **iter);
  int (*disks_init)(struct list_head *table_head);

  void __no_return (*exit)(void);
};

struct platform_block_io {
  int (*read)(struct block_device *dev, uint64_t lba, void *buf, size_t seccount);
};

struct platform {
  int (*init)(void);
  int (*dispose)(void);

  const struct platform_block_io *const block_io;

  const struct platform_terminal *const terminal;
  const struct platform_input *const input;
  const struct platform_time *const time;
  const struct platform_system *const system;
};

/* =========================================================================
 * 2. GLOBAL PLATFORM INSTANCES
 * ========================================================================= */

extern struct platform platform;
extern struct list_head disks;

/* =========================================================================
 * 3. PLATFORM SUBSYSTEM WRAPPERS & HELPERS
 * ========================================================================= */

int no_init(void);

// Input wrappers
int get_key(void);
int poll_key(void);

// Time wrappers
uint64_t get_ticks(void);

// System wrappers
int mmap_iter(struct mem_info *buff, void **iter);
int disks_init(struct list_head *table_head);

#endif
