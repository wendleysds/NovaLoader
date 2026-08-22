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

#ifndef _PATITION_H
#define _PATITION_H

#include <def/attributes.h>
#include <stdint.h>

// mbr
struct MBRPartitionEntry{
	uint8_t status;
	uint8_t chsFirst[3];
	uint8_t type;
	uint8_t chsLast[3];
	uint32_t lbaFirstSector;
	uint32_t totalSectors;
} __packed;

// gpt
struct GPTHeader{
	uint64_t signature;
	uint32_t revision;
	uint32_t headerSize;
	uint32_t headerCRC32;
	uint32_t reserved;
	uint64_t currentLBA;
	uint64_t backupLBA;
	uint64_t firstUsableLBA;
	uint64_t lastUsableLBA;
	uint8_t diskGUID[16];
	uint64_t partEntryStartingLBA;
	uint32_t numPartEntries;
	uint32_t partEntrySize;
	uint32_t partEntryArrayCRC32;
} __packed;

struct GPTPartitionEntry{
	uint8_t typeGUID[16];
	uint8_t uniqueGUID[16];
	uint64_t firstLBA;
	uint64_t lastLBA;
	uint64_t attributes;
	uint16_t name[36];
} __packed;

#endif