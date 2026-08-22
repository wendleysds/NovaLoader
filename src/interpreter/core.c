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

#include <core/interpreter.h>
#include <core/loader.h>
#include <string.h>

#define IS_BLANK(c) ((c) <= ' ' && (c) != '\0')
#define MAX_ARGS 8

static struct var {
    const char *key;
    const char *value;
    struct var *next;
} *vars = NULL;

struct entry *entries = NULL;

struct command *find_command(const char *name) {
    for (struct command *cmd = (struct command *)__commands_start;
        cmd < (struct command *)__commands_end; cmd++) {
        if (strcmp(cmd->name, name) == 0) return cmd;
    }
    return NULL;
}

static int tokenize(char *line, char **argv) {
    int argc = 0;
    char *p = line;

    char *comment = strchr(line, '#');
    if (comment) *comment = '\0';

    while (*p && argc < MAX_ARGS) {
        while (IS_BLANK(*p)) p++;
        if (!*p) break;

        if (*p == '\'' || *p == '"') {
            char quote = *p++;
            argv[argc++] = p;
            while (*p && *p != quote) p++;
            if (!*p) return -EINVAL;
            *p++ = '\0';
        } else {
            argv[argc++] = p;
            while (*p && !IS_BLANK(*p)) p++;
            if (*p) *p++ = '\0';
        }
    }
    return argc;
}

static int exec_with_args(int argc, char **argv) {
	static char errno_buffer[12];
	for (int i = 0; i < argc; i++) {
		if (argv[i][0] == '$') {
			if (argv[i][1] == '?' && argv[i][2] == '\0') {
				dec(errno, errno_buffer);
				argv[i] = errno_buffer;
			} else {
				const char *val = get_var(argv[i] + 1);
				argv[i] = (char *)(val ? val : "");
			}
		}
	}

	if(argc == 1 && strlen(argv[0]) == 0) return 0;

	struct command *cmd = find_command(argv[0]);
	if (!cmd) {
		printf("Unknown command: %s\n", argv[0]);
		return (errno = -ENOEXEC);
	}

	return (errno = cmd->exec(argc, argv));
}

int exec(char *line) {
    char *argv[MAX_ARGS];
    int argc = tokenize(line, argv);
	if (argc <= 0) return argc;
    return exec_with_args(argc, argv);
}

static struct var **find_var(const char *key) {
    struct var **pp = &vars;
    while (*pp && strcmp((*pp)->key, key) != 0) {
        pp = &(*pp)->next;
    }
    return pp;
}

int set_var(const char *key, const char *value) {
    struct var **pp = find_var(key);
    char *new_val = strdup(value);
    if (!new_val) return -ENOMEM;

    if (*pp) {
        free((void *)(*pp)->value);
        (*pp)->value = new_val;
        return 0;
    }

    struct var *v = malloc(sizeof(struct var));
    if (!v) { free(new_val); return -ENOMEM; }

    v->key = strdup(key);
    if (!v->key) { free(v); free(new_val); return -ENOMEM; }

    v->value = new_val;
    v->next = vars;
    vars = v;
    return 0;
}

const char *get_var(const char *key) {
    struct var **pp = find_var(key);
    return *pp ? (*pp)->value : NULL;
}

#if SET_CMD == __y

static int set(int argc, char **argv) {
    if (argc < 3) return -EINVAL;
    return set_var(argv[1], argv[2]);
}

REGISTER_COMMAND("set", set);

#endif

// ================= //

#if SCRIPT_SUPPORT == __y

static char script_buffer[SCRIPT_MAX_SIZE];

static char *trim(char *str) {
	while (IS_BLANK(*str)) str++;
	if (!*str) return str;
	char *end = str + strlen(str) - 1;
	while (end > str && IS_BLANK(*end)) *end-- = '\0';
	return str;
}

static char *get_next_line(char **cursor) {
	if (!cursor || !*cursor || !**cursor) return NULL;

	char *start = *cursor;
	char *p = strchr(start, '\n');

	if (p) {
		*p = '\0';
		*cursor = p + 1;
	} else {
		*cursor = start + strlen(start);
	}

	size_t len = strlen(start);
	if (len > 0 && start[len - 1] == '\r') {
		start[len - 1] = '\0';
	}
	return start;
}

static int parse_script() {
	char *cursor = script_buffer;
	char *line;
	
	#if MENU_CMD == __y
		struct entry *current_entry = NULL;
	#endif

	while ((line = get_next_line(&cursor)) != NULL) {
		char *comment = strchr(line, '#');
		if (comment) *comment = '\0';

		#if MENU_CMD == __y
			if (current_entry) {
				if (strcmp(trim(line), "}") == 0) {
					current_entry->e_end = line;
					current_entry = NULL;
				}
				continue;
			}
		#endif

		char *argv[MAX_ARGS];
		int argc = tokenize(line, argv);

		if (argc < 0) return argc;
		if (argc == 0) continue;

		if (strcmp(argv[0], "entry") == 0) {
			#if MENU_CMD == __y
				if (argc < 2) return -EINVAL;
				if (find_entry(argv[1])) return -EEXIST;

				struct entry *new_entry = malloc(sizeof(struct entry));
				if (!new_entry) return -ENOMEM;

				new_entry->name = argv[1];
				new_entry->e_start = cursor;
				new_entry->e_end = NULL;
				new_entry->next = NULL;

				new_entry->next = entries;
				entries = new_entry;

				current_entry = new_entry;
			#else
				while ((line = get_next_line(&cursor)) != NULL) {
					if (strcmp(trim(line), "}") == 0) break;
				}
				continue;
			#endif
		} else {
			int res = exec_with_args(argc, argv);
			if (res != 0) return res;
		}
	}

	return 0;
}

#if MENU_CMD == __y
	struct entry *find_entry(const char *name) {
		for (struct entry *e = entries; e; e = e->next) {
			if (strcmp(e->name, name) == 0) return e;
		}
		return NULL;
	}

	int run_entry(const struct entry *e) {
		char *ptr = e->e_start;
		char line_buf[256];

		while (ptr < e->e_end) {
			char *next = ptr + strlen(ptr) + 1;
			if (*ptr != '\0') {
				strncpy(line_buf, ptr, sizeof(line_buf) - 1);
				line_buf[sizeof(line_buf) - 1] = '\0';

				int res = exec(line_buf);
				if (res != 0) return res;
			}
			ptr = next;
		}

		return 0;
	}
#endif

static int exec_cmd(int argc, char** argv){
	if(argc < 2) return -EINVAL;

	int res = 0;

	FILE *file = fopen(argv[1]);
	if(IS_ERR(file)) return PTR_ERR(file);

	if(file->size >= SCRIPT_MAX_SIZE) {
		fclose(file);
		return -E2BIG;
	}

	res = load_file_segment(file, 0, script_buffer, SCRIPT_MAX_SIZE);
	if(res < 0){
		fclose(file);
		return res;
	}

	script_buffer[file->size] = '\0';

	res = parse_script();
	fclose(file);
	return res;
}

REGISTER_COMMAND("exec", exec_cmd);
#endif