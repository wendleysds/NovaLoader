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
#include <core/interpreter.h>
#include <string.h>

int parse_path(const char* path, struct path_result* result){
	const char* dev = get_var("root");
	if (path[0] == '(') {
		dev = path + 1;
		char *end = strchr(path, ')');

		if (!end){
			return -EINVAL;
		}

		*end = '\0';
		path = end + 1;
	}

	struct block_device* bdev = bdev_get_by_name(dev);
	if(!bdev){
		return -ENODEV;
	}

	result->path = path;
	result->dev = bdev;

	return SUCCESS;
}