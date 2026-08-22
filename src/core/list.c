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

#include <lib/list.h>

void list_add(struct list_head *new, struct list_head *head) {
	new->next = head->next;
	new->prev = head;
	head->next->prev = new;
	head->next = new;
}

void list_remove(struct list_head *entry) {
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
	entry->next = entry;
	entry->prev = entry;
}