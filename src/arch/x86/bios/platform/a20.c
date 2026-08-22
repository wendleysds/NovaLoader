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

#include <def/errno.h>
#include <biosregs.h>
#include <stddef.h>
#include <ports.h>

#define TRIES 32

#define TRIES_8042  0xFFFF
#define MAX_8042_FF 0x20

static int alery_enabled(){
	uint8_t port_a = inb_p(0x92);
	return (port_a & 0x02);
}

static int empty_8042(void)
{
	uint8_t status;
	int loops = TRIES_8042;
	int ffs   = MAX_8042_FF;

	while (loops--) {
		status = inb_p(0x64);
		if (status == 0xFF) {
			if (!--ffs)
				return 1; // Assume no KBC present
		}
		if (status & 1) {
			inb_p(0x60);
		} else if (!(status & 2)) {
			return 0;
		}
	}

	return 1;
}

static void from_bios(void){
	struct biosregs ireg;
	initregs(&ireg);
	ireg.ax = 0x2401;
	intcall(0x15, &ireg, NULL);
}

static void from_kbd_controller(void){
	empty_8042();

	outb(0xd1, 0x64); // Write command
	empty_8042();

	outb(0xdf, 0x60); // a20
	empty_8042();

	outb(0xff, 0x64); // NULL command
	empty_8042();
}

static void from_port(void){
	uint8_t port_a = inb(0x92);
	port_a |= 0x02;
	port_a &= ~0x01; // disable reset
	outb(port_a, 0x92);
}

int enable_a20(void){
	if(alery_enabled()){
		return SUCCESS;
	}

	int c = TRIES, kbc_err;
	while(c--){
		from_port();
		if(alery_enabled()){
			return SUCCESS;
		}
		
		from_bios();
		if(alery_enabled()){
			return SUCCESS;
		}

		kbc_err = empty_8042();

		if(!kbc_err){
			from_kbd_controller();
			if(alery_enabled()){
				return SUCCESS;
			}
		}	
	}

	return 1;
}