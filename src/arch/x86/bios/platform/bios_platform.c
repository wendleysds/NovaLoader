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

#include "internal.h"

struct platform platform = {
	.init = enable_a20,
	.block_io = &bios_block_io,
	.terminal = &bios_platform_terminal,
	.input = &bios_platform_input,
	.time = &bios_platform_time,
	.system = &bios_platform_system,
};