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

#if LOADER_LINUX == __y
#include <core/loader.h>
#include <string.h>

#include "linux.h"

#define SETUP_LOAD_ADDR   0x90000
#define CMDLINE_LOAD_ADDR 0x98000
#define INITRD_LOAD_ADDR  0x400000

#define SETUP_VERSION 0x206
#define SETUP_HDR     0x53726448 // HdrS

extern __no_return __regparm(2) void linux_16_boot(uint16_t segment, uint16_t entry_point);

static int read_setup(struct setup_header* header, FILE *file){
	if((errno = fseek(file, 0x1F1, SEEK_SET)) < 0) {
		return errno;
	}

	if((errno = fread(file, (char*)header, sizeof(struct setup_header))) < 0) {
		return errno;
	}

	return 0;
}

static int linux_probe(FILE *file){
	struct setup_header header;
	if(read_setup(&header, file)){
		return 1;
	}

	if(header.boot_flag != 0xAA55 || header.header != SETUP_HDR || header.version < SETUP_VERSION){
		return 1;
	}

	return 0;
}

static int __optimize("O0") linux_load(FILE *file, void **out_ctx, char *args[]){
	struct setup_header header;
	if(read_setup(&header, file)){
		return errno;
	}

	size_t setup_sects = header.setup_sects ? header.setup_sects : 4;
	size_t setup_size = (setup_sects + 1) * 512;
	size_t kernel_size = file->size - setup_size;

	size_t kernel_load_addr = (header.loadflags & 0x01) ? 0x100000 : 0x10000;

	errno = load_file_segment(file, 0, (char*)SETUP_LOAD_ADDR, setup_size);
	if(errno < 0) return errno;

	errno = load_file_segment(file, setup_size, (char*)kernel_load_addr, kernel_size);
	if(errno < 0) return errno;

	char *cmdline_buf = (char*)CMDLINE_LOAD_ADDR;
	cmdline_buf[0] = '\0';

	if (args) {
		for (int i = 0; args[i] != NULL; i++) {
			if (i > 0) strcat(cmdline_buf, " ");
			strcat(cmdline_buf, args[i]);
		}
	}

	size_t cmdline_len = strlen(cmdline_buf);
	if (cmdline_len > 0) {
		header.cmd_line_ptr = (uint32_t)CMDLINE_LOAD_ADDR;
		header.cmdline_size = cmdline_len + 1;
	}

	// the loader_id is reserved but we don't care right?
	header.loadflags |= 0x80; // CAN_USE_HEAP
	header.type_of_loader = 0xE0;   // Nibble
	header.ext_loader_type = 0xCF; // CaFe
	header.ext_loader_ver = 0x01;  // 1

	header.heap_end_ptr = 0x8000;
	header.hardware_subarch = X86_SUBARCH_PC;

	memcpy((char*)SETUP_LOAD_ADDR + 0x1F1, &header, sizeof(struct setup_header));

	*out_ctx = (void*)1;
	return 0;
}

static __optimize("O0") int linux_load_initrd(void *ctx, FILE *file, char *args[]){
	struct setup_header *header = (struct setup_header*)(SETUP_LOAD_ADDR + 0x1F1);

	errno = load_file_segment(file, 0, (char*)INITRD_LOAD_ADDR, file->size);
	if(errno < 0) return errno;

	header->ramdisk_image = INITRD_LOAD_ADDR;
	header->ramdisk_size = file->size;

	return 0;
}

static int linux_boot(void *ctx){
	linux_16_boot(SETUP_LOAD_ADDR >> 4, 0x0200);
	__builtin_unreachable();
}

static struct loader_ops linux_ops = {
	.probe = linux_probe,
	.load = linux_load,
	.load_initrd = linux_load_initrd,
	.boot = linux_boot,
};

REGISTER_LOADER(linux, "linux", &linux_ops);

#endif
