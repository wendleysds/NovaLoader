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
#include <biosregs.h>

static uint64_t bios_get_ticks(void){
	uint32_t ticks;

	struct biosregs ireg, oreg;
	initregs(&ireg);
	ireg.ah = 0x02;
	intcall(0x1A, &ireg, &oreg);

	ticks = oreg.dx;
	ticks <<= 16;
	ticks |= oreg.cx;

	return ticks;
}

const struct platform_time bios_platform_time = {
	.init = no_init,
	.get_ticks = bios_get_ticks
};