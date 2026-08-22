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
#include <stdarg.h>

void printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char buf[32], *s;

    for (; *fmt; fmt++) {
        if (*fmt != '%') { putc(*fmt); continue; }

        s = buf;
        switch (*++fmt) {
            case 'c': putc(va_arg(args, int)); continue;
            case 's': s = va_arg(args, char*); break;
            case 'd':
            case 'x': itoa(va_arg(args, long), buf, *fmt == 'd' ? 10 : 16); break;
            default:  putc('%'); putc(*fmt); continue;
        }

        while (*s) putc(*s++);
    }

    va_end(args);
}
