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

static int __path(struct path_result *result, const char *path){
	int res = 0;
	if((res = parse_path(path, result))){
		return res;
	}

	if((res = try_mount(result->dev))){
		printf("error: no filesystem\n");
		return res;
	}

	return res;
}

FILE *fopen(const char *path){
	struct path_result res;
	int err = __path(&res, path);
	if(err){
		return ERR_PTR(err);
	}

	fs_super_t* fs = res.dev->mounted;
	FILE* file = fs->driver->open(fs, res.path);
	if(!IS_ERR(file)){
		file->driver = fs->driver;
	}

    return file;
}

int list_dirs(const char *path){
	struct path_result res;
	int err = __path(&res, path);
	if(err){
		return err;
	}

	fs_super_t* fs = res.dev->mounted;
	return fs->driver->list(fs, res.path);
}

int fread(FILE *file, char *buffer, size_t size){
	return file->driver->read(file, buffer, size);
}

int fseek(FILE *file, int offset, int whence){
    return file->driver->seek(file, offset, whence);
}

int fclose(FILE *file){
	int res = file->driver->close(file);
	free(file);
    return res;
}

int mount(struct block_device *dev){
    if(dev->mounted) return -EBUSY;

	fs_super_t* super;

	for(
		struct fs_driver* driver = (struct fs_driver*)__fsdriver_start; 
		driver < (struct fs_driver*)__fsdriver_end; driver++)
	{
		super = driver->probe(dev);
		if(!IS_ERR(super)){
			super->driver = driver;
			dev->mounted = super;
			return 0;
		}
	}

    return -ENODEV;
}
