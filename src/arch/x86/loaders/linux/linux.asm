#if LOADER_LINUX == __y
[BITS 16]

section .text
global linux_16_boot

linux_16_boot:
	cli

	mov fs, ax
	mov gs, ax
	mov ds, ax
	mov es, ax

	mov ss, ax
	mov sp, 0xFFFE

	xor si, si

	push ax
	push dx

	retf

#endif