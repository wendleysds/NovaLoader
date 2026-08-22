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

#include <malloc.h>

void *memset(void *ptr, int c, size_t size){
    unsigned char *temp = (unsigned char*) ptr;

    for(unsigned long i = 0; i < size; i++){
        temp[i] = (char) c;
    }

    return ptr;
}

void *memcpy(void *dest, const void *src, size_t size){
    unsigned char *d = (unsigned char*)dest;
    unsigned const char *s = (unsigned const char*)src;

    if(d < s){
        while(size--){
            *d++ = *s++;
        }
    }
    else{
        d += size;
        s += size;

        while(size--) {
            *(--d) = *(--s);
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t size){
    unsigned char *c1 = (unsigned char*) s1;
    unsigned char *c2 = (unsigned char*) s2;

    while(size-- > 0){
        if(*c1++ != *c2++){
            return c1[-1] < c2[-1] ? -1 : 1;
        }
    }

    return 0;
}

size_t strnlen(const char *s, size_t n){
	int i = 0;
	while(i < n){
		if(!s[i++]) break;
	}
	return i;
}

size_t strlen(const char *s){
	const char *p;
	for (p = s; *p; ++p);
	return (p - s);
}

char *strndup(const char *s, size_t size) {
	char *copy = (char *)malloc(size + 1);
	if (copy) {
		memcpy(copy, s, size);
		copy[size] = '\0';
	}
	return copy;
}

int strcmp(const char *s1, const char *s2){
	while (*s1 && *s1 == *s2) ++s1, ++s2;
	return (*s1 > *s2) - (*s2  > *s1);
};

char *strchr(const char *s, int c){
	for (; *s != '\0' && *s != c; ++s);
	return *s == c ? (char *) s : NULL;
}

char *strdup(const char *s) {
	return strndup(s, strlen(s));
}

char* strncpy(char *restrict d, const char *restrict s, size_t n){
    char* r = d;
	for(int i = 0; i < n - 1; i++) {
		if(!*s) break;
		*d++ = *s++;
	}
	*d = '\0';
	return r;
}

char* strcpy(char *restrict dst, const char *restrict src){
	size_t len = strlen(src);
	memcpy((void*)dst, (void*)src, len);
	dst[len] = '\0';

	return dst;
}

char* strcat(char *restrict dst, const char *restrict src){
	strcpy(dst + strlen(dst), src);
	return dst;
}