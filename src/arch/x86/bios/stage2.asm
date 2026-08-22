[BITS 16]

global _start

global boot_drvnum

extern main

extern __bss_start
extern __bss_end

section .text.start

_start:
	mov sp, 0x8000
	mov bp, sp

	mov word [boot_drvnum], dx

.enter_unreal:
	cli
	push ds

	lgdt [gdt_descriptor]

	mov eax, cr0
	or al, 1
	mov cr0, eax
	jmp 0x08:.l1

.l1:
	mov bx, 0x10
	mov ds, bx

	and al, 0xFE
	mov cr0, eax
	jmp 0x00:.l2

.l2:
	pop ds
	sti

.clear_bss:
	cld
	xor ax, ax
	mov di, __bss_start
	mov cx, __bss_end
	sub cx, di
	shr cx, 2
	rep stosd

	call main
	jmp $

section .rodata

gdt_descriptor:
dw gdt_end - gdt - 1
dd gdt

gdt:        dd 0,0
codedesc:   db 0xff, 0xff, 0, 0, 0, 10011010b, 00000000b, 0
flatdesc:   db 0xff, 0xff, 0, 0, 0, 10010010b, 11001111b, 0
gdt_end:

boot_drvnum: dw 0