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

void putc(char c) {
    platform.terminal->putc(c);
}

void clear(void) {
    platform.terminal->clear();
}

void set_cursor(int x, int y) {
    platform.terminal->set_cursor(x, y);
}

void get_cursor(int *x, int *y) {
    platform.terminal->get_cursor(x, y);
}

void get_resolution(int *w, int *h) {
    platform.terminal->get_resolution(w, h);
}

int get_key(void) {
    return platform.input->get_key();
}

int poll_key(void) {
    return platform.input->poll_key();
}

uint64_t get_ticks(void) {
    return platform.time->get_ticks();
}

int mmap_iter(struct mem_info *buff, void* *iter) {
    return platform.system->mmap_iter(buff, iter);
}

int disks_init(struct list_head *table_head) {
    return platform.system->disks_init(table_head);
}

void __no_return exit(void){
    platform.system->exit();
}

int no_init(void){ 
    return 0; 
}
