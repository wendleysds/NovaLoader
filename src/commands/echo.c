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

#if ECHO_CMD == __y
#include <core/interpreter.h>
#include <core/platform.h>

static int echo(int argc, char** argv){
	for(int i = 1; i < argc; i++){
		printf("%s ", argv[i]);
	}

	putc('\n');
	return 0;
}

REGISTER_COMMAND("echo", echo);
#endif