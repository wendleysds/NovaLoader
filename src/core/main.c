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
#include <core/interpreter.h>
#include <string.h>

#define INIT(__name) \
	if((errno = platform.__name->init())) {  \
		panic(#__name); \
	} \

LIST_HEAD(disks);

static __no_return void shell(void);

static int __disks(){
	INIT_LIST_HEAD(&disks);

	if((errno = disks_init(&disks))){
		return errno;
	}

	return list_empty(&disks);
}

int main(void){
	malloc_init();

	if((errno = platform.init())){
		panic("init");
	}

	INIT(system);
	INIT(terminal);
	INIT(time);
	INIT(input);

	if((errno = __disks())){
		panic("disks");
	}

	#if SCRIPT_SUPPORT == __y
		exec("exec " DEFAULT_CONFIG_SCRIPT_PATH);
	#endif

	shell();
}

static __no_return void shell(){
	char buff[128];
	size_t pos = 0;
	int c;

	printf("% ");
	for (;;) {
		c = get_key();
		if (!c) continue;

		if (c == '\n' || c == '\r') {
			buff[pos] = '\0';

			putc('\n');
			exec(buff);

			pos = 0;
			printf("% ");
			continue;
		}

		if (c == '\b' || c == 127) {
			if (pos) {
				pos--;
				buff[pos] = '\0';
				printf("\b \b");
			}
			continue;
		}

		if (pos == sizeof(buff) - 1)
			continue;

		buff[pos++] = (char)c;
		putc(c);
	}
}

void __no_return panic(const char *restrict str){
	printf("\nPANIC(%d): %s\n", errno, str);
	while(1) asm volatile("pause" ::: "memory");
}