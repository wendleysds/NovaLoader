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
#include <core/loader.h>

static struct loaded_image {
	const struct loader *loader;
	void *ctx;
} current_image = {0};

static int has_kernel(){
	if(!current_image.ctx){
		printf("Load a kernel\n");
		return 0;
	}

	return 1;
}

static int load_cmd(int argc, char** argv) {
	if(argc < 2) return -EINVAL;

	FILE *file = fopen(argv[1]);
	if(IS_ERR(file)) return PTR_ERR(file);

	int res = OK;
	for (const struct loader *l = (const struct loader *)__loaders_start; l < (const struct loader *)__loaders_end; l++) {
		if (l->ops->probe && l->ops->probe(file) == 0) {
			if(current_image.ctx && current_image.loader->ops->unload){
				current_image.loader->ops->unload(current_image.ctx);
			}

			res = l->ops->load(file, &current_image.ctx, argv + 2);
			if(res != 0){
				printf("Failed\n");
				goto out;
			}

			current_image.loader = l;
			goto out;
		}
	}

	printf("No loader\n");
out:
	fclose(file);
	return res;
}

static int initrd_cmd(int argc, char** argv) {
	if(argc < 2) return -EINVAL;

	if(!has_kernel()) return -EINVAL;

	if(!current_image.loader->ops->load_initrd){
		return -ENOSYS;
	}

	FILE* file = fopen(argv[1]);
	if(IS_ERR(file)){
		return PTR_ERR(file);
	}

	int res = current_image.loader->ops->load_initrd(current_image.ctx, file, argv + 2);
	if(res != 0){
		printf("Failed\n");
	}

	fclose(file);
	return res;
}

static int boot_cmd(int argc, char** argv){
	return has_kernel() ?
		current_image.loader->ops->boot(current_image.ctx) : 
		-EINVAL;
}

REGISTER_COMMAND("load", load_cmd);
REGISTER_COMMAND("initrd", initrd_cmd);
REGISTER_COMMAND("boot", boot_cmd);
