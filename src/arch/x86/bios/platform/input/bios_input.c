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

static int _int16(struct biosregs* oreg, int ah){
	struct biosregs ireg;
	initregs(&ireg);
	ireg.ah = ah;
	intcall(0x16, &ireg, oreg);
	return !(oreg->eflags & (1UL << 6));
}

static int kbd_pending(void){
	struct biosregs oreg;
	return _int16(&oreg, 0x1);
}

static int _get_key(void){
	struct biosregs oreg;
	_int16(&oreg, 0x0);
	return oreg.al;
}

static int _poll_key(void){
	if(kbd_pending()){
		return _get_key();
	}

	return 0;
}

const struct platform_input bios_platform_input = {
	.init = no_init,
	.get_key = _get_key,
	.poll_key = _poll_key,
};
