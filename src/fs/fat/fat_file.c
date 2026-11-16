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

#include "internal.h"

#define PATH_MAX 4096

#define MAX_EXTENSION 3
#define DIR_ENTRY_SIZE 11
#define ILLEGAL_CHARS_REPLACE '_'

typedef int (*fat_dir_cb_t)(struct fat_dir *dir, const char *long_name, struct FATLegacyEntry *entry, void *data);

struct qstr {
	const char *name;
	size_t size;
};

struct lookup_ctx {
    struct qstr *name;
    char formated_name[11];
    struct fat_dir *result;
};

static int path_iterate(const char **cursor, struct qstr *comp){
	const char *path = *cursor;

	while (*path == '/')
		path++;

	if (*path == '\0') {
		return 0;
	}

	comp->name = path;

	while (*path != '/' && *path != '\0')
		path++;

	comp->size = path - comp->name;

	*cursor = path;

	return 1;
}

static inline char _convert_char(char c) {
	if (c < 0x20 || c == '"' || c == '*' || c == '+' || c == ',' || c == '.' ||
		c == '/' || c == ':' || c == ';' || c == '<' || c == '=' || c == '>' ||
		c == '?' || c == '[' || c == '\\' || c == ']') {
		return ILLEGAL_CHARS_REPLACE;
	}

	return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

static void fat_name_generate_short(const char *name, size_t name_size, char *out) {
	int i, j = 0;
	int dot_idx = -1;

	for (i = 0; i < 11; i++)
		out[i] = ' ';
	for (i = 0; i < name_size; i++) {
		if (name[i] == '.')
			dot_idx = i;
	}

	for (i = 0; i < name_size && i != dot_idx && j < 8; i++) {
		out[j++] = _convert_char(name[i]);
	}

	if (dot_idx != -1) {
		j = 8;
		for (i = dot_idx + 1; i < name_size && j < 11; i++) {
			out[j++] = _convert_char(name[i]);
		}
	}
}

static inline int _cmp_len(const char *s1, const char *s2, size_t len) {
	for (size_t i = 0; i < len; i++) {
		if (s1[i] != s2[i])
			return 0; // false
	}
	return 1; // true
}

static int fat_is_eof(struct fat *fat, uint32_t cluster){
	switch (fat->type) {
		case FAT_TYPE_12: return cluster >= 0x0FF8;
		case FAT_TYPE_16: return cluster >= 0xFFF8;
		case FAT_TYPE_32: return cluster >= 0x0FFFFFF8;
		default: return 1;
	}
}

static uint32_t fat_get_eof(struct fat *fat){
	switch (fat->type) {
		case FAT_TYPE_12: return 0x0FF8;
		case FAT_TYPE_16: return 0xFFF8;
		case FAT_TYPE_32: return 0x0FFFFFF8;
		default: return 0x0FFFFFF8;
	}
}

static uint32_t next_cluster(struct fat *fat, uint32_t cluster) {
	uint32_t fatOffset = 0;
	switch (fat->type) {
		case FAT_TYPE_12:
			fatOffset = cluster + (cluster / 2);
			break;
		case FAT_TYPE_16:
			fatOffset = cluster * 2;
			break;
		case FAT_TYPE_32:
			fatOffset = cluster * 4;
			break;
		default:
			return fat_get_eof(fat);
	}

	uint32_t bytesPerSec = fat->headers.boot.bytesPerSec;
	uint32_t fatSecNum = fat->headers.boot.rsvdSecCnt + (fatOffset / bytesPerSec);
	uint32_t entOffset = fatOffset % bytesPerSec;

	uint8_t sector[1024];
	uint8_t sectorsToRead = (fat->type == FAT_TYPE_12 && (entOffset == bytesPerSec - 1)) ? 2 : 1;

	if(disk_read(fat->dev, fatSecNum, sector, sectorsToRead)){
		return fat_get_eof(fat);
	}

	uint32_t val = *(uint16_t*)&sector[entOffset];
	switch (fat->type) {
		case FAT_TYPE_12:
			if (cluster & 1) {
				val >>= 4;
			} else {
				val &= 0x0FFF;
			}
			break;
		case FAT_TYPE_16:
			val &= 0xFFFF;
			break;
		case FAT_TYPE_32:
			val = (*(uint32_t*)&sector[entOffset]) & 0x0FFFFFFF;
			break;
		default: return fat_get_eof(fat);
	}

	return val;
}

static uint32_t cluster_to_lba(struct fat *fat, uint32_t cluster) {
	return first_data_sector(fat) + ((cluster - 2) * fat->headers.boot.secPerClus);
}

static void fat_process_long_name(struct FATLongNameEntry *lentry, char *long_name) {
	int idx = ((lentry->ord & 0x3F) - 1) * 13;
	if (idx < 0 || idx >= 256)
		return;

	for (int k = 0; k < 5; k++)
		long_name[idx++] = (char)lentry->name1[k];
	for (int k = 0; k < 6; k++)
		long_name[idx++] = (char)lentry->name2[k];
	for (int k = 0; k < 2; k++)
		long_name[idx++] = (char)lentry->name3[k];

	if (lentry->ord & 0x40)
		long_name[idx] = '\0';
}

static int fat_iterate_dir(struct fat_dir *dir, fat_dir_cb_t cb, void *data) {
    struct fat *fat = dir->fat;
    const size_t max_entries = fat->headers.boot.bytesPerSec / sizeof(struct FATLegacyEntry);
    
    uint8_t sector[512];
    char long_name[256] = {0};
    
    uint32_t current_cluster = dir->cluster;
    do {
        if (fat_is_eof(fat, current_cluster)) break;

        uint32_t lba;
        if (fat->type != FAT_TYPE_32 && !dir->parent) {
            lba = dir->sector;
        } else {
            lba = cluster_to_lba(fat, current_cluster);
        }

        for (uint32_t sec = 0; sec < fat->headers.boot.secPerClus; sec++) {
            if (disk_read(fat->dev, lba + sec, sector, 1)) 
                return -EIO;

            for (size_t i = 0; i < max_entries; i++) {
                struct FATLegacyEntry *entry =
                        (struct FATLegacyEntry *)&sector[i * sizeof(struct FATLegacyEntry)];

                if (entry->name[0] == 0x00)
                    return 0;
                if (entry->name[0] == 0xE5)
                    continue; 

                if ((entry->attr & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME) {
                    fat_process_long_name((struct FATLongNameEntry *)entry, long_name);
                    continue;
                }

                int ret = cb(dir, long_name, entry, data);
                if (ret != 0) {
                    return ret;
                }

                long_name[0] = '\0';
            }
        }
    } while ((current_cluster = next_cluster(fat, current_cluster)));

    return 0;
}

static int fat_lookup_cb(struct fat_dir *dir, const char *long_name, struct FATLegacyEntry *entry, void *data) {
    struct lookup_ctx *ctx = data;

    int matches = _cmp_len((const char *)entry->name, ctx->formated_name, DIR_ENTRY_SIZE) ||
        (long_name[0] != '\0' && (
            _cmp_len(long_name, ctx->name->name, ctx->name->size) &&
            long_name[ctx->name->size] == '\0'
        ));

    if (matches) {
        struct fat_dir *new_dir = malloc(sizeof(struct fat_dir));
        if (!new_dir) return -ENOMEM;

        new_dir->fat = dir->fat;
        new_dir->cluster = (entry->fstClusHI << 16) | entry->fstClusLO;
        new_dir->parent = dir;
        new_dir->size = entry->fileSize;
        new_dir->attr = entry->attr;
        INIT_LIST_HEAD(&new_dir->children);
        INIT_LIST_HEAD(&new_dir->sibling);

        new_dir->name = strndup(ctx->name->name, ctx->name->size);
        if (!new_dir->name) {
            free(new_dir);
            return -ENOMEM;
        }

		// Cache
        list_add_tail(&new_dir->sibling, &dir->children);
        list_add_tail(&new_dir->dirs, &dir->fat->dirs);
        
        ctx->result = new_dir;
        return 1;
    }
    
    return 0;
}

static struct fat_dir *fat_lookup(struct fat_dir *dir, struct qstr *name) {
    // 1. search in cache
    struct fat_dir *pos = 0x0;
    list_for_each_entry(pos, &dir->children, sibling) {
        if (pos->name && pos->name[0] == name->name[0]) {
            if (_cmp_len(pos->name, name->name, name->size) &&
                    pos->name[name->size] == '\0') {
                return pos;
            }
        }
    }

    // 2. search in disk
    struct lookup_ctx ctx = { .name = name, .result = NULL };
    fat_name_generate_short(name->name, name->size, ctx.formated_name);

    int err = fat_iterate_dir(dir, fat_lookup_cb, &ctx);
    
    if (err < 0) return ERR_PTR(err);
    if (ctx.result) return ctx.result;
    
    return ERR_PTR(-ENOENT);
}

static void fat_print_entry_name(const char *long_name, const struct FATLegacyEntry *entry) {
	if(long_name[0] != '\0'){
		printf(long_name);
	}else{
		char tmp[12];
		memcpy(tmp, entry->name, 11);
		tmp[11] = '\0';
		printf(tmp);
	}
	printf("  ");
}

static int fat_list_cb(struct fat_dir *dir, const char *long_name, struct FATLegacyEntry *entry, void *data) {
    fat_print_entry_name(long_name, entry);
    return 0;
}

static struct fat_dir* fat_find_dir(struct fat *fat, const char *path){
	size_t path_len = strnlen(path, PATH_MAX);
    if (path_len >= PATH_MAX) {
        return ERR_PTR(-EINVAL);
    }

    struct fat_dir *current_dir = &fat->root;

    if (path_len > 0 && !(path[0] == '/' && path[1] == '\0')) {
        struct qstr comp;
        const char *cursor = path;

        while (path_iterate(&cursor, &comp)) {
            struct fat_dir *next_dir = fat_lookup(current_dir, &comp);
            if (IS_ERR(next_dir)) {
                return ERR_CAST(next_dir);
            }
            current_dir = next_dir;
        }
    }

	return current_dir;
}

int fat_list(fs_super_t* super, const char *path) {
    struct fat_dir* dir = fat_find_dir(super->private, path);
	if(IS_ERR(dir)){
		return PTR_ERR(dir);
	}

    int err = fat_iterate_dir(dir, fat_list_cb, NULL);
    return err < 0 ? err : 0;
}

FILE *fat_open(fs_super_t* super, const char *path){
	struct fat_dir* dir = fat_find_dir(super->private, path);
	if(IS_ERR(dir)){
		return ERR_CAST(dir);
	}

	if((dir->attr & ATTR_ARCHIVE) == 0){
		return ERR_PTR(-EINVAL); // It's a directory, not a file
	}

	struct fat_file *file = malloc(sizeof(struct fat_file));
	if (!file) {
		return ERR_PTR(-ENOMEM); // Memory allocation failed
	}

	file->dentry = dir;
	file->pos = 0;
	file->startCluster = dir->cluster;
	file->currentCluster = dir->cluster;
	file->size = dir->size;
	file->base.size = dir->size;

	return (FILE*)file;
}

int fat_read(FILE *_file, char *buffer, size_t size) {
	struct fat_file *file = (struct fat_file*)_file;
    if (file->pos >= file->size) return 0;

    struct fat *fat = file->dentry->fat;
    uint32_t remaining = (uint32_t)size;

    if (remaining > file->size - file->pos) {
        remaining = file->size - file->pos;
    }

    int err = 0;

    if (remaining == 0) return 0;

    uint32_t clusterSize = fat->headers.boot.bytesPerSec * fat->headers.boot.secPerClus;
    uint32_t cluster = file->currentCluster;
	uint8_t *bounceBuf = fat->bounceBuf;
    uint32_t totalRead = 0;

    while (remaining > 0) {
        uint32_t offInCluster = file->pos % clusterSize;
        uint32_t bytesLeftInCluster = clusterSize - offInCluster;
        uint32_t toCopy = (remaining < bytesLeftInCluster) ? remaining : bytesLeftInCluster;

        uint32_t lba = cluster_to_lba(fat, cluster);

        if (offInCluster == 0 && toCopy == clusterSize) {
            if ((err = disk_read(fat->dev, lba, (uint8_t*)buffer + totalRead, fat->headers.boot.secPerClus))) {
                return err;
            }
        } 
        else {
            if (!bounceBuf) return -EINVAL; 

            if ((err = disk_read(fat->dev, lba, bounceBuf, fat->headers.boot.secPerClus))) {
                return err;
            }

            memcpy(buffer + totalRead, bounceBuf + offInCluster, toCopy);
        }

        file->pos += toCopy;
        totalRead += toCopy;
        remaining -= toCopy;

        if ((file->pos % clusterSize) == 0) {
            cluster = next_cluster(fat, cluster);
            
            if (fat_is_eof(fat, cluster)) { 
                break;
            }

            file->currentCluster = cluster;
        }
    }

    return totalRead;
}

int fat_seek(FILE* _file, int offset, int whence){
	struct fat_file *file = (struct fat_file*)_file;
    uint32_t filesize = file->size;
    int64_t target_pos;

    switch(whence){
        case SEEK_SET:
            target_pos = offset;    
            break;
        case SEEK_CUR:
            target_pos = (int64_t)file->pos + offset;
            break;
        case SEEK_END:
            target_pos = (int64_t)filesize + offset;
            break;
        default:
            return -EINVAL;
    }

    if (target_pos < 0 || target_pos > filesize) {
        return -ERANGE;
    }

    uint32_t target = (uint32_t)target_pos;
    struct fat *fat = file->dentry->fat;
    uint32_t clusterSize = fat->headers.boot.bytesPerSec * fat->headers.boot.secPerClus;

    if (filesize == 0) {
        file->pos = 0;
        file->currentCluster = file->startCluster;
        return 0;
    }

    uint32_t effectiveTarget = (target == filesize) ? target - 1 : target;
    uint32_t targetClusterIdx = effectiveTarget / clusterSize;
    uint32_t currentClusterIdx = file->pos / clusterSize;

    uint32_t cluster;
    uint32_t startIdx;

    if (target >= file->pos && file->currentCluster != 0 && !fat_is_eof(fat, file->currentCluster)) {
        cluster = file->currentCluster;
        startIdx = currentClusterIdx;
    } else {
        cluster = file->startCluster;
        startIdx = 0;
    }

    for (uint32_t i = startIdx; i < targetClusterIdx; i++) {
        cluster = next_cluster(fat, cluster);
        if (fat_is_eof(fat, cluster)) {
            return -EIO;
        }
    }

    file->currentCluster = cluster;
    file->pos = target;

    return 0;
}

int fat_close(FILE *_file){
	return 0;
}
#endif