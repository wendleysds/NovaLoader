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

#ifndef _STDIO_H
#define _STDIO_H

#include <def/attributes.h>

void putc(char c);
void printf(const char *restrict fmt, ...);

void clear(void);
void set_cursor(int x, int y);
void get_cursor(int *x, int *y);
void get_resolution(int *w, int *h);

void __no_return panic(const char *restrict str);

#endif
