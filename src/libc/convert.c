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

char* itoa(int value, char *buf, int radix) {
	char *p1 = buf, *p2;
	unsigned int uval;
	int negative = 0, i;

	negative = (radix != 2 && value < 0);
	if (negative) {
		uval = (unsigned int)(-value);
	} else {
		uval = (unsigned int)value;
	}

	do {
		i = uval % radix;
		uval /= radix;
		if (i < 10)
			*p1++ = i + '0';
		else
			*p1++ = i + 'A' - 10;
	} while (uval > 0);

	if (negative && radix == 10) {
		*p1++ = '-';
	}
	
	*p1 = '\0';

	p2 = p1 - 1;
	p1 = buf;
	while (p1 < p2) {
		char tmp = *p1;
		*p1++ = *p2;
		*p2-- = tmp;
	}

	return p1;
}

int atoi(const char *str, int radix) {
	int c, result = 0;

    while ((c = *str++)) {
        c = (c >= 'a') ? c - 'a' + 10 :
            (c >= 'A') ? c - 'A' + 10 : c - '0';

        if (c < 0 || c >= radix) break;

        result = result * radix + c;
    }

    return result;
}