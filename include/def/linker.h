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

#ifndef _LINKER_H
#define _LINKER_H

extern char __text_start[];
extern char __text_end[];
extern char __rodata_start[];
extern char __rodata_end[];
extern char __data_start[];
extern char __data_end[];
extern char __fsdriver_start[];
extern char __fsdriver_end[];
extern char __commands_start[];
extern char __commands_end[];
extern char __loaders_start[];
extern char __loaders_end[];
extern char __bss_start[];
extern char __bss_end[];

#endif
