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

#ifndef _FS_H
#define _FS_H

#include <stddef.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

struct block_device;
struct fs_driver;

typedef struct {
	void* private;
	struct fs_driver* driver;
	size_t size;
} FILE;

typedef struct {
	void* private;
	struct fs_driver* driver;
} fs_super_t;

struct fs_driver {
	const char* name;
	fs_super_t* (*probe)(struct block_device*);
	FILE* (*open)(fs_super_t*, const char* path);
	int (*list)(fs_super_t*, const char* path);
	int (*read)(FILE*, char*, size_t);
	int (*seek)(FILE*, int, int);
	int (*close)(FILE*);
};

#define REGISTER_FS_DRIVER(__name, __probe, __open, __list, __read, __seek, __close) \
    static const struct fs_driver fs_driver_##__name \
    __attribute__((used, section(".fsdriver"))) = { \
		.name = #__name, \
		.probe = __probe, \
		.list = __list, \
		.open = __open, \
		.read = __read, \
		.seek = __seek, \
		.close = __close,\
    }

struct path_result { const char* path; struct block_device* dev; };
int parse_path(const char* path, struct path_result* result);

int mount(struct block_device *dev);

FILE *fopen(const char *path);
int list_dirs(const char *path);
int fread(FILE *, char *buffer, size_t size);
int fseek(FILE *, int offset, int whence);
int fclose(FILE *);

#endif