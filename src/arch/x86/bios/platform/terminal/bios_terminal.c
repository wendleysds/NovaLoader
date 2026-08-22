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

static struct terminal_ctx {
	int width;
	int height;
	int page;
} ctx;

static int _init(void) {
	struct biosregs ireg, oreg;
	initregs(&ireg);
	ireg.ah = 0x0F;

	intcall(0x10, &ireg, &oreg);

	ctx.width = oreg.ah;
	ctx.page  = oreg.bh;

	if (ctx.width == 80) {
		ctx.height = 25;
	} else if (ctx.width == 40) {
		ctx.height = 25;
	} else if(ctx.width == 80 && oreg.al == 0x0) {
		ctx.height = 50;
	}else{
		ctx.height = 25;
	}

	return 0;
}

static void _putc(char c) {
	if (c == '\n') {
		_putc('\r');
	}

	struct biosregs r;
	initregs(&r);
	r.ah = 0x0E;
	r.al = (uint8_t)c;
	r.bh = (uint8_t)ctx.page;
	r.bl = 0x07;
	
	intcall(0x10, &r, NULL);
}

static void _set_cursor(int x, int y) {
	struct biosregs ireg;
	initregs(&ireg);

	ireg.ah = 0x02;
	ireg.bh = (uint8_t)ctx.page;
	ireg.dh = (uint8_t)y;
	ireg.dl = (uint8_t)x;

	intcall(0x10, &ireg, NULL);
}

static void _clear_screen(void) {
	struct biosregs ireg;
	initregs(&ireg);

	ireg.ah = 0x06;
	ireg.bh = 0x07;
	ireg.dh = (uint8_t)(ctx.height - 1);
	ireg.dl = (uint8_t)(ctx.width - 1);

	intcall(0x10, &ireg, NULL);

	_set_cursor(0, 0);
}

static void _get_cursor(int* x, int* y) {
	struct biosregs ireg, oreg;
	initregs(&ireg);

	ireg.ah = 0x03;
	ireg.bh = (uint8_t)ctx.page;

	intcall(0x10, &ireg, &oreg);

	if (x) *x = oreg.dl;
	if (y) *y = oreg.dh;
}

static void _get_resolution(int* x, int* y) {
	if (x) *x = ctx.width;
	if (y) *y = ctx.height;
}

const struct platform_terminal bios_platform_terminal = {
	.init           = _init,
	.putc           = _putc,
	.clear          = _clear_screen,
	.get_cursor     = _get_cursor,
	.set_cursor     = _set_cursor,
	.get_resolution = _get_resolution
};