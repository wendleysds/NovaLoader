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

#ifndef _FS_INTERNAL_H
#define _FS_INTERNAL_H

#include <def/attributes.h>
#include <lib/list.h>
#include <core/fs.h>
#include <stdint.h>

#define FAT_INVAL 0xFFFFFFFF

#define FAT_SIGNATURE 0x29
#define FAT_ENTRY_SIZE 0x02
#define FAT_BAD_SECTOR 0xFF7
#define FAT_UNUSED 0x00

// FAT File/Directory attributes
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

#define ATTR_LONG_NAME \
	(ATTR_READ_ONLY | ATTR_HIDDEN \
	| ATTR_SYSTEM |ATTR_VOLUME_ID)

#define ATTR_LONG_NAME_MASK \
	(ATTR_READ_ONLY | ATTR_HIDDEN \
	|ATTR_SYSTEM |ATTR_VOLUME_ID \
	|ATTR_DIRECTORY |ATTR_ARCHIVE)

struct FATHeader{
	uint8_t jmpBoot[3];
	uint8_t OEMName[8];
	uint16_t bytesPerSec;
	uint8_t secPerClus;
	uint16_t rsvdSecCnt;
	uint8_t numFATs;
	uint16_t rootEntCnt;
	uint16_t totSec16;
	uint8_t mediaType;
	uint16_t FATSz16;
	uint16_t secPerTrk;
	uint16_t numHeads;
	uint32_t hiddSec;
	uint32_t totSec32;
} __packed;

struct FATHeaderExtended{
	uint8_t drvNum;
    uint8_t reserved1;
    uint8_t bootSig;
    uint32_t volID;
    uint8_t volLab[11];
    uint8_t filSysType[8];
} __packed;

struct FAT32HeaderExtended{
	uint32_t FATSz32;
	uint16_t extFlags;
	uint16_t FSVer;
	uint32_t rootClus;
	uint16_t FSInfo;
	uint16_t BkBootSec;
	uint8_t reserved[12];
	uint8_t drvNum;
	uint8_t reserved1;
	uint8_t bootSig;
	uint32_t volID;
	uint8_t volLab[11];
	uint8_t filSysType[8];
} __packed;

struct FATLegacyEntry{
	uint8_t name[11];
	uint8_t attr;
	uint8_t NTRes;
	uint8_t crtTimeTenth;
	uint16_t crtTime;
	uint16_t crtDate;
	uint16_t lstAccDate;
	uint16_t fstClusHI;
	uint16_t wrtTime;
	uint16_t wrtDate;
	uint16_t fstClusLO;
	uint32_t fileSize;
} __packed;

struct FATLongNameEntry{
	uint8_t ord;
	uint16_t name1[5];
	uint8_t attr;
	uint8_t type;
	uint8_t chksum;
	uint16_t name2[6];
	uint16_t fstClusLO;
	uint16_t name3[2];
} __packed;

struct FATHeaders{
	struct FATHeader boot;
	union {
		struct FATHeaderExtended fat12;
		struct FATHeaderExtended fat16;
		struct FAT32HeaderExtended fat32;
	};
};

struct fat_dir {
	const char *name; // aways dup null terminated

	struct fat *fat;
	union {
		uint32_t cluster;
		uint32_t sector;
	};
	uint32_t size;
	uint8_t attr;

	struct fat_dir *parent;
	struct list_head children;
	struct list_head sibling;

	struct list_head dirs;
};

typedef enum {
	FAT_TYPE_12,
	FAT_TYPE_16,
	FAT_TYPE_32,
	FAT_INVALID,
	FAT_TYPES_COUNT
} fat_type_t;

struct fat {
	fat_type_t type;
	struct FATHeaders headers;
	struct block_device *dev;
	uint8_t *bounceBuf;
	struct fat_dir root;
	struct list_head dirs;
};

struct fat_file {
	FILE base;

	uint32_t pos;
	uint32_t startCluster;
	uint32_t currentCluster;
	uint32_t size;
	struct fat_dir *dentry;
};

uint32_t root_dir_sectors(struct fat *fat);
uint32_t fat_sectors(struct fat *fat);
uint32_t first_data_sector(struct fat *fat);

int fat_list(fs_super_t* super, const char *path);
FILE *fat_open(fs_super_t* super, const char *path);
int fat_read(FILE* _file, char *buffer, size_t size) ;
int fat_seek(FILE* _file, int offset, int whence);
int fat_close(FILE *_file);

#endif