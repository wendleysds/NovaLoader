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

#if MENU_CMD == __y
#include <core/platform.h>
#include <core/interpreter.h>
#include <string.h>

#define DEFAULT_TIMEOUT 30

#define KEY_ENTER 0xD
#define KEY_UP    'w'
#define KEY_DOWN  's'
#define KEY_SHELL 'c'
#define NO_KEY    0

static const char* menu_header = "======= NovaLoader =======";
static const char* menu_footer = "==========================";
static const char* menu_help =   " w: up; s: dw; enter: sel ";

static int get_key_timeout(int secs){
    int start, current, key;

    start = get_ticks();

    while (secs) {
        key = poll_key();
        if(key) return key;

        current = get_ticks();
        if (start != current) {
            secs--;
            start = current;
        }
    }

    return 0;
}

static int print_center(int x_res, const char* str) {
    int len = strlen(str);
    int padding = (x_res - len) / 2 ;

    for (int i = 0; i < padding; i++) {
        putc(' ');
    }

    printf("%s\n", str);
    return len;
}

static inline void draw_selection(int x, int start_y, int index, char ch) {
    set_cursor(x, start_y + index);
    putc(ch);
}

static int boot_menu (int argc, char** argv) {
    if(!entries) return 1;

    int count = 0;
    const char* default_name = get_var("default");
    int selected_index = 0;

    for (struct entry* cur = entries; cur; cur = cur->next) {
        if (default_name && strcmp(default_name, cur->name) == 0) {
            selected_index = count;
        }
        count++;
    }

    clear();

    int x_res, y_res;
    get_resolution(&x_res, &y_res);

    int total_height = count + 4; // header + footer + 2 spaces
    int start_y = (y_res / 2) - (total_height / 2);

    set_cursor(0, start_y);

    print_center(x_res, menu_header);
    putc('\n');

    int entries_start_y = start_y + 2;
    int max_len = 0;

    int i = 0;
    char line_buf[256];
    for (struct entry* cur = entries; cur; cur = cur->next, i++) {
        dec(i + 1, line_buf);

        strcat(line_buf, ". ");
        strcat(line_buf, cur->name);

        int text_len = print_center(x_res, line_buf);
        if (text_len > max_len) max_len = text_len;
    }

    putc('\n');
    print_center(x_res, menu_footer);
    print_center(x_res, menu_help);

    int cursor_x = (x_res / 2) + (max_len / 2) + 1;
    draw_selection(cursor_x, entries_start_y, selected_index, '*');

    const char* timeout_str = get_var("timeout");
    int timeout = timeout_str ? atoi(timeout_str, 10) : DEFAULT_TIMEOUT;
    if (timeout > 99) timeout = 99;

    int timeout_x = (x_res / 2);
    int timeout_y = start_y + total_height + 1;

    while(timeout >= 0){
        int k = get_key_timeout(1);

        if (k != NO_KEY) {
            if (k == KEY_ENTER) break;

            int prev_index = selected_index;

            if (k == KEY_DOWN && selected_index + 1 < count) {
                selected_index++;
            } else if (k == KEY_UP && selected_index > 0) {
                selected_index--;
            }

            if (prev_index != selected_index) {
                draw_selection(cursor_x, entries_start_y, prev_index, ' ');
                draw_selection(cursor_x, entries_start_y, selected_index, '*');
            }
        }

        if(k == KEY_SHELL){
			clear();
            return 0;
        }

        if(k == NO_KEY){
            set_cursor(timeout_x, timeout_y);

            if (timeout < 10) {
                putc('0');
            }

			printf("%d", timeout);

            timeout--;
        }
    }

    struct entry* cur = entries;
    for (i = 0; i < selected_index && cur; i++) {
        cur = cur->next;
    }

    clear();

    return run_entry(cur);
}

REGISTER_COMMAND("menu", boot_menu);
#endif