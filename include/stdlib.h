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

#ifndef _STDLIB_H
#define _STDLIB_H

#include <def/attributes.h>
#include <malloc.h>
#include <stddef.h>

void itoa(int value, char *buf, int radix);
int atoi(const char *str, int radix);

#define hex(value, buffer) itoa((value), (buffer), 16)
#define dec(value, buffer) itoa((value), (buffer), 10)
#define oct(value, buffer) itoa((value), (buffer), 8)
#define bin(value, buffer) itoa((value), (buffer), 2)

void __no_return exit(void);

#endif
