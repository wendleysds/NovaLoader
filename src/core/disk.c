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
#include <string.h>

struct block_device *bdev_get_by_name(const char *name) {
  struct block_device *bdev;
  list_for_each_entry(bdev, &disks, list) {
    if (strcmp(bdev->name, name) == 0) {
      return bdev;
    }
  }

  return NULL;
}

int try_mount(struct block_device *dev) {
  if (!dev->mounted) {
    if (mount(dev)) {
      return -ENODEV;
    }
  }

  return OK;
}

int disk_read(struct block_device *dev, uint64_t lba, void *buf, size_t seccount) {
  if (lba > UINT64_MAX - dev->start_lba) {
    return -EOVERFLOW;
  }
  uint64_t start = lba + dev->start_lba;

  if (seccount > dev->total_lba || start > (dev->start_lba + dev->total_lba) - seccount) {
    return -ERANGE;
  }

  return platform.block_io->read(dev, start, buf, seccount);
}