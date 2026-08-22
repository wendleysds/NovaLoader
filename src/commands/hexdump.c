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

#if HEXDUMP_CMD == __y
#include <core/interpreter.h>
#include <core/platform.h>
#include <string.h>

void __hexdump(const void *start, size_t size) {
    const uint8_t *p = start;
    int suppressed = 0;

    for (size_t i = 0; i < size; i += 16) {
        size_t len = (size - i < 16) ? (size - i) : 16;

        if (i >= 16 && len == 16 && memcmp(p + i, p + i - 16, 16) == 0) {
            if (!suppressed) {
                printf("*\n");
                suppressed = 1;
            }
            continue;
        }

        suppressed = 0;

		printf("%x: ", p + i);
        for (size_t j = 0; j < 16; j++) {
            if (j < len) {
                u8 byte = p[i + j];

                if (byte < 0x10)
					putc('0');

				printf("%x ", byte);
            } else {
                printf("   ");
            }
        }

        printf(" |");

        for (size_t j = 0; j < len; j++) {
            u8 byte = p[i + j];
            if (byte >= 32 && byte <= 126) {
                putc(byte);
            } else {
                putc('.');
            }
        }

        printf("|\n");
    }
}

static int hexdump(int argc, char **argv) {
	if (argc < 2)
		return -EINVAL;

	__hexdump(
		(void*)atoi(argv[1], 16),
		atoi(argv[2], 10)
	);

	return 0;
}

REGISTER_COMMAND("hex", hexdump);
#endif
