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
#include <string.h>

int load_file_segment(FILE *file, size_t offset, void* dest, size_t size){
	char buffer[512];
	int bytes_read = 0;
	size_t total = 0;
	uint8_t *ptr = (uint8_t*)dest;

	if((bytes_read = fseek(file, offset, SEEK_SET)) < 0){
		goto out;
	}

	while((bytes_read = fread(file, buffer, sizeof(buffer))) > 0){
		if(total + bytes_read > size){
			bytes_read = size - total;
		}

		memcpy(ptr + total, buffer, bytes_read);
		total += bytes_read;

		if(total >= size) break;
	}

out:
	return bytes_read < 0 ? bytes_read : (int)total;
}
