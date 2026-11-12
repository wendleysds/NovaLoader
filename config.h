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

// -- const

#define __y 1
#define __n 0

// -- Helpers

#define KiB(x) ((x) * 1024UL)
#define MiB(x) ((x) * KiB(1024))

#define ALIGN_UP(x, a)   (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

// --- CPU --- //

#define CPU_ALIGNMENT 2

// --- File Systems --- //

#define FAT_SUPPORT __y

// --- Loaders --- //

#define LOADER_LINUX __y
#define LOADER_WEYOS __y

// --- Scripts --- //

#define SCRIPT_SUPPORT __y

#define DEFAULT_CONFIG_SCRIPT_PATH "/boot/boot.cfg"
#define DEFAULT_CONFIG_SCRIPT_PARTITION 1 // on boot device

#define SCRIPT_MAX_SIZE KiB(1)

// --- Commands --- //

#define SET_CMD __y
#define HEXDUMP_CMD __y
#define LS_CMD __y
#define ECHO_CMD __y
#define MENU_CMD __y
#define EXIT_CMD __y