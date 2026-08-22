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

#if FAT_SUPPORT == __y
#include <stdint.h>

#include "internal.h"

uint32_t root_dir_sectors(struct fat *fat) {
	if(fat->type == FAT_TYPE_32) return 0;
	uint32_t rootBytes = fat->headers.boot.rootEntCnt * 32;
	uint32_t bytesPerSec = fat->headers.boot.bytesPerSec;
	return (rootBytes + bytesPerSec - 1) / bytesPerSec;
}

uint32_t fat_sectors(struct fat *fat) {
	uint32_t fatSize = fat->headers.boot.FATSz16;
	if (fatSize == 0) {
		fatSize = fat->headers.fat32.FATSz32;
	}

	return fat->headers.boot.rsvdSecCnt + (fat->headers.boot.numFATs * fatSize);
}

uint32_t first_data_sector(struct fat *fat) {
	return fat_sectors(fat) + root_dir_sectors(fat);
}
#endif