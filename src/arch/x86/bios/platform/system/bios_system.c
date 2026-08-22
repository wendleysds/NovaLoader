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

#include <core/interpreter.h>
#include <biosregs.h>
#include <string.h>

#include "../internal.h"

extern const uint16_t boot_drvnum;

struct e820_entry {
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
} __packed;

static void __no_return _exit(void) {
	asm volatile("int $0x18\n1: hlt\njmp 1b");
	__builtin_unreachable();
}

static int _map_iterate(struct mem_info *mem, void **iter) {
	struct e820_entry entry;
	struct biosregs ireg, oreg;

	initregs(&ireg);
	ireg.ax  = 0xe820;
	ireg.cx  = sizeof(entry);
	ireg.edx = 0x534d4150;
	ireg.ebx = (uint32_t)(uintptr_t)*iter;
	ireg.di  = (size_t)&entry;

	intcall(0x15, &ireg, &oreg);

	if ((oreg.eflags & 1) || oreg.eax != 0x534d4150)
		return -1;

	mem->base = entry.base_addr;
	mem->size = entry.length;
	mem->reserved = (entry.type != 1);
	mem->atype = entry.type;

	*iter = (void *)(uintptr_t)oreg.ebx;
	return !!oreg.ebx;
}

static struct block_device* bdev_alloc(int drv, int part) {
	struct block_device* dev = malloc(sizeof(struct block_device));
	if (!dev) return NULL;

	memset(dev, 0, sizeof(*dev));
	INIT_LIST_HEAD(&dev->partitions);
	INIT_LIST_HEAD(&dev->list);

	char *p = dev->name;
	*p++ = (drv & 0x80) ? 'h' : 'f';
	*p++ = 'd';
	*p++ = 'a' + (drv & 0x7F);

	if (part) {
		dec(part, p);
	}

	if (drv == boot_drvnum && (part == DEFAULT_CONFIG_SCRIPT_PARTITION || part == 0)) {
		set_var("root", dev->name);
	}

	return dev;
}

static int _init_disks(struct list_head *table_head) {
	uint8_t sec0[512];

	for (int i = 0x80; i <= 0x8F; i++) {
		if (bios_disk_exists(i) <= 0) continue;

		struct block_device* dev = bdev_alloc(i, 0);
		if (!dev) return -ENOMEM;

		if (bios_init_disk(i, dev) || disk_read(dev, 0, sec0, 1)) {
			if (dev->private) free(dev->private);
			free(dev);
			continue;
		}

		list_add_tail(&dev->list, table_head);

		struct MBRPartitionEntry* mbrs = (void*)(sec0 + 0x1BE);
		for (int j = 0; j < 4; j++) {
			if (!mbrs[j].totalSectors) continue;

			struct block_device* part_dev = bdev_alloc(i, j + 1);
			if (part_dev) {
				part_dev->private = dev->private;
				part_dev->start_lba = mbrs[j].lbaFirstSector;
				part_dev->total_lba = mbrs[j].totalSectors;
				list_add_tail(&part_dev->partitions, &dev->partitions);
				list_add_tail(&part_dev->list, table_head);
			}
		}
	}

	return SUCCESS;
}

const struct platform_system bios_platform_system = {
	.init = no_init,
	.mmap_iter = _map_iterate,
	.disks_init = _init_disks,
	.exit = _exit,
};
