# NovaLoader

A lightweight, modular, and configurable x86 bootloader for BIOS/MBR, developed in C and NASM Assembly. The project was designed to boot educational or custom kernels and operating systems, featuring support for FAT12, FAT16, and FAT32, multiple kernel loaders, configuration scripts, and an interactive interface.

![C](https://img.shields.io/badge/Language-C-blue.svg)
![Assembly](https://img.shields.io/badge/Language-Assembly-green.svg)
![Architecture](https://img.shields.io/badge/Arch-x86-orange.svg)
![Platform](https://img.shields.io/badge/Platform-BIOS%2FMBR-red.svg)

---

## Overview

NovaLoader utilizes a **two-stage bootloader** architecture:

1. **Stage 1**: Boot code executed in Real Mode, responsible for locating and loading Stage 2.
2. **Stage 2**: The main core of the bootloader, developed in C/Assembly, responsible for platform initialization, disk and file system access, script interpretation, interactive commands, and kernel loading.

The architecture is organized into layers, including HAL/Platform, block device manager, VFS, file systems, kernel loaders, and a command interpreter.

---

## Key Features

### Two-stage architecture

- **Stage 1 (MBR - 512 bytes)**:
- Executed in Real Mode. 
- Locates and loads Stage 2.
- **Stage 2**:
- Bootloader core. 
- Initializes HAL, VFS, command interpreter, and loaders. 
- Uses C and Assembly code. 
- Operates using low-level x86 platform resources.

### FAT Support

- Support for **FAT12, FAT16, and FAT32**.
- Unified implementation in `src/fs/fat/`. - Automatic detection of the FAT type based on the volume's cluster count.
- File system access via the VFS layer.

### Modular architecture

Components are decoupled and can be registered using linker sections:

- `REGISTER_FS_DRIVER`
- `REGISTER_LOADER`
- `REGISTER_COMMAND`

The architecture allows for the separation of:

- HAL/Platform
- Block device manager
- VFS
- File system drivers
- Kernel loaders
- Commands
- Script interpreter
- Freestanding LibC

### Disk and path detection

- Detection and registration of hard disks/floppy disks.
- MBR partition support.
- Device identifiers such as:
- `(hda)`
- `(hda1)`
- `(hdb)`
- Support for paths with device prefixes:
- `(hda1)/boot/kernel`
- The `ls` command can list detected disks and partitions or the contents of a directory.

### Automatic loader selection

The `load` command can **probe** the loaded image and test registered loaders.

This allows for automatic identification of the kernel format without requiring the user to explicitly specify the loader type.

Supported loaders include:

- Linux `bzImage`
- WeyOS `bzImage`

### Kernel support

#### Linux

- Linux boot protocol.
- `bzImage` format.
- Command-line arguments (`cmdline`).
- `initrd` loading.

#### WeyOS

- Support for the custom WeyOS boot protocol.
- `initrd` loading.

### Interpreter and scripts

NovaLoader features a command interpreter capable of executing commands directly in the shell or via configuration files.

The default configuration file is:

```text
/boot/boot.cfg
```

The interpreter offers:

- Environment variables. - Variable expansion (`$var`).
- Exit code of the last command (`$?`).
- `entry` blocks.
- Execution of external scripts via `exec`.
- Interactive commands.

### Interactive Interface

- Integrated shell for debugging and manual command execution.
- Text-mode boot menu.
- Keyboard navigation.
- Boot entry selection.
- Countdown (`timeout`) for automatic booting.

### Freestanding LibC and HAL

The project features an internal, independent LibC suitable for a freestanding environment, with functionality such as:

- `printf`
- `malloc`
- Strings
- Lists
- Basic I/O

The HAL provides abstractions for platform resources, including:

- VGA/ANSI video.
- Keyboard.
- Timing.
- A20 line management.
- BIOS/x86-specific registers and resources.

---

## Configuration

NovaLoader features two-level configuration.

### Compile-time Configuration

The `config.h` file allows enabling or disabling bootloader features, including:

- FAT support.
- Specific loaders.
- Shell commands.
- `ls`
- `hex`
- `menu`
- `echo`
- Slab/Arena memory managers.
- Other optional features.

### Runtime Configuration

The `boot.cfg` file defines boot behavior.

Example:

```text
set default linux
set timeout 10

entry linux {
	echo 'Loading Linux Kernel...'
	load /boot/kernel 'console=ttyS0 quiet'
	echo 'Loading Initrd...'
	initrd /boot/ramfs
	boot
}

entry weyos {
	echo 'Loading WeyOS...'
	load /boot/weyos_kernel
	boot
}

menu
```

---

## Scripting Language

### Supported commands

| Command | Description | Example |
|---|---|---|
| `set <key> <value>` | Sets an environment variable | `set timeout 15` |
| `echo <message>` | Prints a message, with `$var` support | `echo $default` |
| `load <path> [args]` | Detects the loader and loads a kernel | `load /boot/kernel 'root=/dev/sda1'` |
| `initrd <path>` | Loads a RAMDisk/Initrd | `initrd /boot/ramfs` |
| `boot` | Boots the loaded kernel | `boot` |
| `entry <name> { ... }` | Defines a boot entry | `entry linux { ... }` |
| `menu` | Displays the selection menu | `menu` |
| `exec <path>` | Executes an external `.cfg` script | `exec /boot/other.cfg` |
| `ls [path]` | Lists disks, partitions, or directories | `ls /boot` |
| `hex <memory> <amount>` | Displays a memory region in hexadecimal | `hex 7c00 256` |
| `clear` | Clears the screen | `clear` |
| `exit` | Exits the shell | `exit` | ---

## Project Structure

```text
novaloader/
├── boot.cfg               # Default boot configuration script
├── config.h               # Build configurations and features
├── linker.ld              # Linker script
├── Makefile               # Build system
├── install                # Image/MBR/FAT installation utility
│
├── include/
│   ├── core/              # Platform APIs, VFS, disks, and loaders
│   ├── def/               # Definitions, error codes, and attributes
│   └── lib/               # LibC headers and helper structures
│
├── src/
│   ├── arch/x86/
│   │   ├── bios/          # Stage 1, Stage 2, BIOS, A20, and drivers
│   │   └── loaders/       # Linux bzImage and WeyOS loaders
│   │
│   ├── commands/          # Interpreter commands
│   ├── core/              # VFS, HAL, disks, and bootloader core
│   ├── fs/fat/            # FAT12/16/32 drivers
│   ├── interpreter/       # Command and script interpreter
│   └── libc/              # printf, malloc, string, lists, etc.
│
└── tools/                 # Helper utilities
```

---

## Prerequisites

To build and test NovaLoader, the following are required:

- **NASM**: x86 assembler.
- **GCC / GCC Cross-Compiler**:
- `i686-elf-gcc`, or
- GCC with `-m16` and `-ffreestanding` support.
- **GNU Binutils**:
- `i686-elf-ld`
- `i686-elf-objcopy`
- **Python 3**: used by FAT image installation/formatting utilities.
- **QEMU**: optional, for execution and testing.

---

## Building

Build the project with:

```bash
make
```

The build process includes:

1. Compiling `stage1.bin` in Assembly. 2. Compilation of Stage 2 C/Assembly components.
3. Generation of `stage2.elf` and conversion to `stage2.bin`.

Build artifacts can be found in the `build/` directory, as per the project configuration.

To clean up build files:

```bash
make clear
```

---

## Execution with QEMU

The following tutorial shows how to build NovaLoader, create and format a FAT disk image, install the bootloader, populate the filesystem, and boot the image using QEMU.

### 1. Build NovaLoader

Build the bootloader first:

```bash
make
```

This builds the bootloader and its required components.

### 2. Create a Disk Image

Create a 16 MiB empty disk image:

```bash
dd if=/dev/zero of=disk.img bs=512 count=32768 conv=sync
```

The resulting image contains:

```text
32768 sectors × 512 bytes = 16 MiB
```

### 3. Format the Disk

Format the image with the filesystem supported by NovaLoader:

```bash
python3 tools/fat.py format --start-lba 1 --end-lba 32767 --partition 1 disk.img
```

This prepares the image so that files can be stored and accessed through the NovaLoader VFS/FAT layer.

### 4. Install NovaLoader

Install NovaLoader into the disk image:

```bash
python3 install -p 1 disk.img
```

This installs the bootloader and prepares the image for booting.

### 5. Create `boot.cfg`

A `boot.cfg` file is optional if you want to boot manually from the NovaLoader shell. For an automated boot, create `/boot/boot.cfg`.

Example:

```text
set default linux  # Selects `linux` as the default entry
set timeout 10     # Sets a 10-second menu timeout

entry linux {      # Defines a boot entry named `linux`
    echo 'Loading Linux Kernel...'

    load /boot/linux # Loads the kernel	

    echo 'Loading Initrd...'

    initrd /boot/initramfs # Loads the initrd

    boot # Boots the loaded kernel
}

menu # Displays the boot menu
```

### 6. Populate the Disk Image

Copy the boot configuration, Linux kernel, and initrd into the FAT filesystem:

```bash
python3 tools/fat.py put disk.img boot.cfg /boot/boot.cfg
python3 tools/fat.py put disk.img bzImage /boot/linux
python3 tools/fat.py put disk.img ramfs /boot/initramfs
```

After this step, the image should contain:

```text
/boot/
├── stage2.bin
├── boot.cfg
├── linux
└── initramfs
```

The names used inside `boot.cfg` must match the paths of the files stored in the image.

### 7. Boot with QEMU

Run the disk image using QEMU:

```bash
qemu-system-i386 -drive format=raw,file=disk.img
```

NovaLoader should start, load `/boot/boot.cfg`, display the configured menu, and load the selected kernel.

### Complete Workflow

The entire process can be performed with:

```bash
make

dd if=/dev/zero of=disk.img bs=512 count=32768 conv=sync

python3 install -p 1 -f 1 32768 disk.img

python3 tools/fat.py put disk.img boot.cfg /boot/boot.cfg
python3 tools/fat.py put disk.img bzImage /boot/linux
python3 tools/fat.py put disk.img ramfs /boot/initramfs

qemu-system-i386 -drive format=raw,file=disk.img
```

---

## Example Boot Flow

A typical NovaLoader flow can be represented as follows:

```text
BIOS
│
▼
Stage 1
│
│ Locates and loads
▼
Stage 2
│
├── Initializes HAL/Platform
├── Detects disks and partitions
├── Initializes VFS
├── Registers file systems
├── Initializes interpreter
│
▼
/boot/boot.cfg
	│
	├── set
	├── entry
	├── load
	├── initrd
	└── boot
	│
	▼
	Kernel
```

---

## Project Philosophy

NovaLoader was developed with a focus on:

- Studying bootloaders and x86 architecture.
- Experimenting with BIOS and MBR.
- Low-level file system development.
- Separation of hardware, storage, VFS, and loaders.
- Modular extensibility.
- Compile-time and runtime configuration.
- Support for different boot protocols.

The project is intended for **educational and experimental** purposes, specifically for studying operating system initialization and x86 architecture.

---

## License

This project is under development for educational purposes and for the study of operating systems and x86 architecture.

NovaLoader is licensed under the MIT License.

See [LICENSE](LICENSE) for the full license text.