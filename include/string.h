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

#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

void *memset(void*, int, size_t);
void *memcpy(void*, const void*, size_t);
int memcmp(const void*, const void*, size_t);

#define memmove memcpy

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t n);
char *strchr(const char *s, int c);
int strcmp(const char *s1, const char *s2);
char *strndup(const char *s, size_t size);
char *strdup(const char *s);
char *strncpy(char *restrict d, const char *restrict s, size_t n);
char* strcpy(char *restrict d, const char *restrict s);
char* strcat(char *restrict d, const char *restrict s);
#endif
