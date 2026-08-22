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

#if LS_CMD == __y
#include <core/platform.h>
#include <core/interpreter.h>

static int ls(int argc, char** argv){
	struct block_device* bdev;
	int res = SUCCESS;

	if(argc == 1){
		list_for_each_entry(bdev, &disks, list){
			printf("(%s) ", bdev->name);
		}

		goto out;
	}

	res = list_dirs(argv[1]);

out:
	putc('\n');
	return res;
};

REGISTER_COMMAND("ls", ls);
#endif