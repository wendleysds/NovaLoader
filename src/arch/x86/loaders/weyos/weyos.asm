#if LOADER_WEYOS == __y
[BITS 16]

section .text
global weyos_16_boot

weyos_16_boot:
	cli

	mov fs, ax
	mov gs, ax
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov sp, 0xFFFF

	push ax
	push dx

	retf
#endif