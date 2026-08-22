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

#ifndef _DISK_H
#define _DISK_H

#include <core/fs.h>
#include <lib/list.h>
#include <stdint.h>

struct block_device {
  char name[16];

  uint64_t start_lba;
  uint64_t total_lba;

  void *private;
  fs_super_t *mounted;

  struct list_head partitions;
  struct list_head list;
};

int disk_read(struct block_device *dev, uint64_t lba, void *buf, size_t seccount);
struct block_device *bdev_get_by_name(const char *name);
int try_mount(struct block_device *dev);

#endif