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

#include <biosregs.h>
#include <string.h>

void initregs(struct biosregs *reg){
	memset(reg, 0, sizeof(*reg));
	reg->eflags |= (((1UL)) << (0));
	reg->ds = ds();
	reg->es = ds();
	reg->fs = fs();
	reg->gs = gs();
}
