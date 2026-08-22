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

#ifndef _INTERPRETER_H
#define _INTERPRETER_H

#include <def/attributes.h>
#include <core/fs.h>

#define REGISTER_COMMAND(cmd, handle) \
	static const struct command __cmd_##handle  __used __section(".commands") = \
	{ \
		.name = cmd, .exec = handle, \
	}

struct command {
	const char* name;
	int (*exec)(int argc, char** argv);
};

struct entry {
	const char *name;
	char *e_start;
	char *e_end;
	struct entry *next;
};

extern struct entry *entries;

struct entry *find_entry(const char *name);
int run_entry(const struct entry *e);

int exec(char* line);

int set_var(const char *name, const char *value);
const char* get_var(const char* name);

#endif