"""
 * SPDX-License-Identifier: MIT
 *
 * NovaLoader - Lightweight, modular, and configurable x86 bootloader for BIOS/MBR
 *
 * Copyright (c) 2026 Wendley Santos
 *
 * This file is part of NovaLoader.
 * See the LICENSE file in the project root for license information.
"""

UNUSED  =  0
READ_ONLY     =  0x01
HIDDEN         =  0x02
SYSTEM         =  0x04
VOLUME         =  0x08
DIRECTORY      =  0x10
ARCHIVE        =  0x20
LONG_NAME      =  READ_ONLY | HIDDEN | SYSTEM | VOLUME
LONG_NAME_MASK =  READ_ONLY | HIDDEN | SYSTEM | VOLUME | DIRECTORY | ARCHIVE

ROOT_DIRECTORY = -1 # special cluster number for root directory in FAT12/16