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
#include <core/platform.h>
#include <string.h>

#include "core/fs.h"
#include "internal.h"

static fat_type_t _fat_get_type(const uint8_t *sector0Buffer){
	const struct FATHeader *bpb = (const struct FATHeader*)sector0Buffer;
	const struct FAT32HeaderExtended *fat32ext = (const struct FAT32HeaderExtended*)(sector0Buffer + sizeof( struct FATHeader));

	if (bpb->bytesPerSec == 0 || bpb->secPerClus == 0){
		return FAT_INVALID;
	}

	uint32_t rootDirSectors = ((bpb->rootEntCnt * 32) + (bpb->bytesPerSec - 1)) / bpb->bytesPerSec;

	uint32_t totSec = (bpb->totSec16 != 0) ? bpb->totSec16 : bpb->totSec32;
	uint32_t fatSz = (bpb->FATSz16 != 0) ? bpb->FATSz16 : fat32ext->FATSz32;

	uint32_t dataSec = totSec - (bpb->rsvdSecCnt + (bpb->numFATs * fatSz) + rootDirSectors);

	uint32_t countOfClusters = dataSec / bpb->secPerClus;

	if (countOfClusters < 4085){
		return FAT_TYPE_12;
	} else if (countOfClusters < 65525){
		if(bpb->FATSz16 != 0){
			return FAT_TYPE_16;
		}
	}

	return FAT_TYPE_32;
}

static int8_t _valid_fat_sector(const uint8_t *sector0){
	if (sector0[0] != 0xEB && sector0[0] != 0xE9) {
		return 0;
	}

	if (sector0[510] != 0x55 || sector0[511] != 0xAA){
		return 0;
	}

	int valid_oem = 1;
	for (int i = 3; i < 11; ++i) {
		if (sector0[i] < 0x20 || sector0[i] > 0x7E) {
			valid_oem = 0;
			break;
		}
	}

	return valid_oem;
}

static struct fat *fat_mount(struct block_device *dev){
	uint8_t sec0[0x200];
	if(disk_read(dev, 0, sec0, 1)){
		return ERR_PTR(-EIO);
	}

	if(!_valid_fat_sector(sec0)){
		return ERR_PTR(-EINVAL);
	}

	fat_type_t fat_type = _fat_get_type(sec0);
	if(fat_type == FAT_INVALID){
		return ERR_PTR(-EINVAL);
	}

	struct fat *fat = malloc(sizeof(struct fat));
	if(!fat){
		return ERR_PTR(-ENOMEM);
	}

	memset(fat, 0x0, sizeof(struct fat));

	fat->dev = dev;
	fat->type = fat_type;
	fat->root.fat = fat;
	INIT_LIST_HEAD(&fat->root.sibling);
	INIT_LIST_HEAD(&fat->root.children);
	INIT_LIST_HEAD(&fat->dirs);
	
	memcpy(&fat->headers, sec0, sizeof(struct FATHeaders));

	fat->bounceBuf = malloc(
		fat->headers.boot.secPerClus * fat->headers.boot.bytesPerSec
	);

	if(!fat->bounceBuf){
		free(fat);
		return ERR_PTR(-ENOMEM);
	}

	if(fat_type == FAT_TYPE_32){
		fat->root.cluster = fat->headers.fat32.rootClus;
	}else{
		fat->root.sector = fat_sectors(fat);
	}

	return fat;
}

fs_super_t* fat_probe(struct block_device *dev){
	fs_super_t* super = malloc(sizeof(fs_super_t));
	if(!super) return ERR_PTR(-ENOMEM);

	struct fat* fat = fat_mount(dev);
	if(IS_ERR(fat)){
		free(super);
		return ERR_CAST(fat);
	}

	super->private = fat;
	return super;
}

REGISTER_FS_DRIVER(FAT, fat_probe, fat_open, fat_list, fat_read, fat_seek, fat_close);

/*
void fat_umount(struct fat *fat){
	free(fat->bounceBuf);

	struct fat_dir *pos, *tmp;
	list_for_each_entry_safe(pos, tmp, &fat->dirs, dirs){
		list_remove(&pos->sibling);
		if(pos->name) free((void*)pos->name);
		free(pos);
	}

	free(fat);
}
*/
#endif