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

#if EXIT_CMD == __y
#include <core/interpreter.h>
#include <core/platform.h>

static int __exit(int argc, char** argv){
    exit();
    __builtin_unreachable();
}

REGISTER_COMMAND("exit", __exit);

#endif