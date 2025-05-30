#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

void *memset(void *restrict dst, int c, size_t n);

void *memcpy(void *restrict dst, const void *restrict src, size_t n);

size_t strlen(const char *s);

size_t strnlen(const char *s, size_t maxlen);

int strcmp(const char *s1, const char *s2);

int strncmp(const char *s1, const char *s2, size_t n);

#endif
