OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
	. = 0x8000;

	.text : ALIGN(4) {
		__text_start = .;
		KEEP(*(.text.start)) 
		*(.text*)
		__text_end = .;
	}

	.rodata : ALIGN(4) {
		__rodata_start = .;
		*(.rodata*)
		__rodata_end = .;
	}

	.data : ALIGN(16) {
		__data_start = .;
		*(.data*)
		__data_end = .;
	}

	.fsdriver : ALIGN(4) {
		__fsdriver_start = .;
		KEEP(*(.fsdriver))
		__fsdriver_end = .;
	}

	.commands : ALIGN(4) {
		__commands_start = .;
		KEEP(*(.commands))
		__commands_end = .;
	}

	.loaders : ALIGN(4) {
		__loaders_start = .;
		KEEP(*(.loaders))
		__loaders_end = .;
	}

	__end_reserved = .;

	.bss : ALIGN(4) {
		__bss_start = .;
		*(COMMON)
		*(.bss*)
		__bss_end = .;
	}
}
