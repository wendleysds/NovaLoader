/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _ASM_X86_BOOTPARAM_H
#define _ASM_X86_BOOTPARAM_H

#include <stdint.h>

/* ram_size flags */
#define RAMDISK_IMAGE_START_MASK	0x07FF
#define RAMDISK_PROMPT_FLAG		0x8000
#define RAMDISK_LOAD_FLAG		0x4000

/* loadflags */
#define LOADED_HIGH	(1<<0)
#define KASLR_FLAG	(1<<1)
#define QUIET_FLAG	(1<<5)
#define KEEP_SEGMENTS	(1<<6)
#define CAN_USE_HEAP	(1<<7)

/* xloadflags */
#define XLF_KERNEL_64			(1<<0)
#define XLF_CAN_BE_LOADED_ABOVE_4G	(1<<1)
#define XLF_EFI_HANDOVER_32		(1<<2)
#define XLF_EFI_HANDOVER_64		(1<<3)
#define XLF_EFI_KEXEC			(1<<4)
#define XLF_5LEVEL			(1<<5)
#define XLF_5LEVEL_ENABLED		(1<<6)
#define XLF_MEM_ENCRYPTION		(1<<7)

#ifndef __ASSEMBLER__

struct setup_header {
	u8	setup_sects;
	u16	root_flags;
	u32	syssize;
	u16	ram_size;
	u16	vid_mode;
	u16	root_dev;
	u16	boot_flag;
	u16	jump;
	u32	header;
	u16	version;
	u32	realmode_swtch;
	u16	start_sys_seg;
	u16	kernel_version;
	u8	type_of_loader;
	u8	loadflags;
	u16	setup_move_size;
	u32	code32_start;
	u32	ramdisk_image;
	u32	ramdisk_size;
	u32	bootsect_kludge;
	u16	heap_end_ptr;
	u8	ext_loader_ver;
	u8	ext_loader_type;
	u32	cmd_line_ptr;
	u32	initrd_addr_max;
	u32	kernel_alignment;
	u8	relocatable_kernel;
	u8	min_alignment;
	u16	xloadflags;
	u32	cmdline_size;
	u32	hardware_subarch;
	u64	hardware_subarch_data;
	u32	payload_offset;
	u32	payload_length;
	u64	setup_data;
	u64	pref_address;
	u32	init_size;
	u32	handover_offset;
	u32	kernel_info_offset;
} __attribute__((packed));

struct sys_desc_table {
	u16 length;
	u8  table[14];
};

/* Gleaned from OFW's set-parameters in cpu/x86/pc/linux.fth */
struct olpc_ofw_header {
	u32 ofw_magic;	/* OFW signature */
	u32 ofw_version;
	u32 cif_handler;	/* callback into OFW */
	u32 irq_desc_table;
} __attribute__((packed));

struct efi_info {
	u32 efi_loader_signature;
	u32 efi_systab;
	u32 efi_memdesc_size;
	u32 efi_memdesc_version;
	u32 efi_memmap;
	u32 efi_memmap_size;
	u32 efi_systab_hi;
	u32 efi_memmap_hi;
};

/*
 * This is the maximum number of entries in struct boot_params::e820_table
 * (the zeropage), which is part of the x86 boot protocol ABI:
 */
#define E820_MAX_ENTRIES_ZEROPAGE 128

/*
 * Smallest compatible version of jailhouse_setup_data required by this kernel.
 */
#define JAILHOUSE_SETUP_REQUIRED_VERS

/**
 * enum x86_hardware_subarch - x86 hardware subarchitecture
 *
 * The x86 hardware_subarch and hardware_subarch_data were added as of the x86
 * boot protocol 2.07 to help distinguish and support custom x86 boot
 * sequences. This enum represents accepted values for the x86
 * hardware_subarch.  Custom x86 boot sequences (not X86_SUBARCH_PC) do not
 * have or simply *cannot* make use of natural stubs like BIOS or EFI, the
 * hardware_subarch can be used on the Linux entry path to revector to a
 * subarchitecture stub when needed. This subarchitecture stub can be used to
 * set up Linux boot parameters or for special care to account for nonstandard
 * handling of page tables.
 *
 * These enums should only ever be used by x86 code, and the code that uses
 * it should be well contained and compartmentalized.
 *
 * KVM and Xen HVM do not have a subarch as these are expected to follow
 * standard x86 boot entries. If there is a genuine need for "hypervisor" type
 * that should be considered separately in the future. Future guest types
 * should seriously consider working with standard x86 boot stubs such as
 * the BIOS or EFI boot stubs.
 *
 * WARNING: this enum is only used for legacy hacks, for platform features that
 *	    are not easily enumerated or discoverable. You should not ever use
 *	    this for new features.
 *
 * @X86_SUBARCH_PC: Should be used if the hardware is enumerable using standard
 *	PC mechanisms (PCI, ACPI) and doesn't need a special boot flow.
 * @X86_SUBARCH_LGUEST: Used for x86 hypervisor demo, lguest, deprecated
 * @X86_SUBARCH_XEN: Used for Xen guest types which follow the PV boot path,
 * 	which start at asm startup_xen() entry point and later jump to the C
 * 	xen_start_kernel() entry point. Both domU and dom0 type of guests are
 * 	currently supported through this PV boot path.
 * @X86_SUBARCH_INTEL_MID: Used for Intel MID (Mobile Internet Device) platform
 *	systems which do not have the PCI legacy interfaces.
 * @X86_SUBARCH_CE4100: Used for Intel CE media processor (CE4100) SoC
 * 	for settop boxes and media devices, the use of a subarch for CE4100
 * 	is more of a hack...
 */
enum x86_hardware_subarch {
	X86_SUBARCH_PC = 0,
	X86_SUBARCH_LGUEST,
	X86_SUBARCH_XEN,
	X86_SUBARCH_INTEL_MID,
	X86_SUBARCH_CE4100,
	X86_NR_SUBARCHS,
};

#endif /* __ASSEMBLER__ */

#endif /* _ASM_X86_BOOTPARAM_H */
