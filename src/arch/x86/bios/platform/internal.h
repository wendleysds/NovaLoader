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

#ifndef _BIOS_PLATFORM_INTERAL
#define _BIOS_PLATFORM_INTERAL

#include <core/platform.h>

int enable_a20(void);

extern const struct platform_block_io bios_block_io;
extern const struct platform_terminal bios_platform_terminal;
extern const struct platform_input bios_platform_input;
extern const struct platform_time bios_platform_time;
extern const struct platform_system bios_platform_system;

int bios_disk_exists(int drive_num);
int bios_init_disk(int drive_num, struct block_device* dev);

#endif