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

#if LOADER_WEYOS == __y
#include <core/loader.h>
#include <string.h>

#include "weyos.h"

#define PAGE_SIZE 4096
#define SETUP_LOAD_ADDR  0x90000
#define INITRD_LOAD_ADDR 0x20000

extern __no_return __regparm(2) void weyos_16_boot(uint16_t segment, uint16_t entry_point);

static int read_setup(struct boot_tag_setup* header, FILE *file){
	if((errno = fseek(file, 0, SEEK_SET)) < 0) {
		return errno;
	}

	if((errno = fread(file, (char*)header, sizeof(struct boot_tag_setup))) < 0) {
		return errno;
	}

	return 0;
}

static int weyos_probe(FILE *file){
	struct boot_tag_setup header;
	if(read_setup(&header, file)){
		return 1;
	}

	if(header.boot_sig != 0xAA55){
		return 1;
	}

	return 0;
}

static int __optimize("O0") weyos_load(FILE *file, void **out_ctx, char *args[]){
	struct boot_tag_setup header;
	if(read_setup(&header, file)){
		return errno;
	}

	uint16_t setup_size = header.setup_sectors * 0x200;
	uint32_t start_kernel = (setup_size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	uint32_t kernel_size = file->size - start_kernel;

	if((header.syssize * 16) < kernel_size){
		return -EINVAL;
	}

	errno = load_file_segment(file, 0, (void*)SETUP_LOAD_ADDR, setup_size);
	if(errno < 0){
		return errno;
	}

	memset(
		(void*)(SETUP_LOAD_ADDR + (header.setup_sectors + 1) * 512),
		0x0,
		(64 - (header.setup_sectors + 1)) * 512
	);

	errno = load_file_segment(file, start_kernel, (void*)header.pref_address, kernel_size);
	if(errno < 0){
		return errno;
	}

	*out_ctx = (void*)header.code16_start;

	return SUCCESS;
}

static int __optimize("O0") weyos_load_initrd(void *ctx, FILE *file, char *args[]){
	struct boot_tag_setup* header = (void*)SETUP_LOAD_ADDR;

	errno = load_file_segment(file, 0, (void*)INITRD_LOAD_ADDR, file->size);
	if(errno < 0){
		return errno;
	}

	header->ramdisk_ptr = INITRD_LOAD_ADDR;
	header->ramdisk_size = file->size;
	
	memcpy((void*)SETUP_LOAD_ADDR, header, sizeof(struct boot_tag_setup));

	return SUCCESS;
}

static int weyos_boot(void *ctx){
	weyos_16_boot(SETUP_LOAD_ADDR >> 4, (uint16_t)(uint32_t)ctx);
	__builtin_unreachable();
}

static struct loader_ops weyos_ops = {
	.probe = weyos_probe,
	.load = weyos_load,
	.load_initrd = weyos_load_initrd,
	.boot = weyos_boot,
};

REGISTER_LOADER(weyos, "weyos", &weyos_ops);

#endif