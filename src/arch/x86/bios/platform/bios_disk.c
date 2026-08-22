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
#include <biosregs.h>

struct geo {
	uint16_t cylinder;
	uint8_t heads;
	uint8_t secPerTrk;
};

struct bios_disk_ctx {
	struct geo geo;
    uint8_t driveNum;
};

static int _int13(struct biosregs* ireg, struct biosregs* oreg) {
    intcall(0x13, ireg, oreg);
    return (oreg->eflags & 1) ? -EIO : SUCCESS;
}

static int _disk_read_chs(struct bios_disk_ctx* bdsk_data, uint64_t lba, void* buffer, size_t seccount) {
    uint32_t lba32 = (uint32_t)lba;
    uint32_t spt = bdsk_data->geo.secPerTrk;
    uint32_t heads = bdsk_data->geo.heads;

    uint32_t tmp = lba32 / spt;
    uint8_t sector = (uint8_t)(lba32 - tmp * spt) + 1;
    uint16_t cyl = (uint16_t)(tmp / heads);
    uint8_t head = (uint8_t)(tmp - cyl * heads);

    struct biosregs ireg, oreg;
    initregs(&ireg);
    ireg.ah = 0x02;
    ireg.al = (uint8_t)seccount;
    ireg.ch = (uint8_t)(cyl & 0xFF);
    ireg.cl = (uint8_t)(sector | ((cyl >> 2) & 0xC0));
    ireg.dh = head;
    ireg.dl = bdsk_data->driveNum;
    ireg.es = SEG(buffer);
    ireg.bx = OFF(buffer);

    return _int13(&ireg, &oreg);
}

static int disk_ctx_init(struct bios_disk_ctx* disk, struct block_device* dev, int drive_num) {
	struct biosregs ireg, oreg;
	initregs(&ireg);
	ireg.ah = 0x08;
	ireg.dl = (uint8_t)drive_num;

	if (_int13(&ireg, &oreg) != SUCCESS) {
		return -EBUSY;
	}

	struct geo* geo = &disk->geo;

	disk->driveNum = (uint8_t)drive_num;
	geo->secPerTrk = oreg.cl & 0x3F;
	geo->heads = (oreg.dh & 0xFF) + 1;
	geo->cylinder = (((oreg.cl & 0xC0) << 2) | (oreg.ch & 0xFF)) + 1;
	dev->total_lba = geo->secPerTrk * geo->heads * geo->cylinder;

	return SUCCESS;
}

static int bios_disk_read(struct block_device* dev, uint64_t lba, void* buffer, size_t seccount) {
	return _disk_read_chs(dev->private, lba, buffer, seccount);
}

int bios_disk_exists(int drive_num) {
    struct biosregs ireg, oreg;
    initregs(&ireg);
    ireg.ah = 0x15;
    ireg.dl = (uint8_t)drive_num;
    intcall(0x13, &ireg, &oreg);

    return !_int13(&ireg, &oreg) && oreg.ah != 0;
}

int bios_init_disk(int drive_num, struct block_device* dev) {
    struct bios_disk_ctx* disk = malloc(sizeof(struct bios_disk_ctx));

	if (!disk) return -ENOMEM;

	if (disk_ctx_init(disk, dev, drive_num)) {
		free(disk);
		return 1;
	}

    dev->private = disk;

    return 0;
}

const struct platform_block_io bios_block_io = {
	.read = bios_disk_read,
};