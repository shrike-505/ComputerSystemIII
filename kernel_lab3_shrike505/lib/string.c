#include <string.h>

void *memset(void *restrict dst, int c, size_t n) {
    for (size_t i = 0; i < n; i++) {
        ((char *)dst)[i] = c;
    }
    return dst;
}

size_t strnlen(const char *restrict s, size_t maxlen) {
    size_t len = 0;
    while (s[len] != '\0') {
        if (len == maxlen) {
            break;
        }
        len++;
    }
    return len;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

size_t strcmp(const char *s1, const char *s2) {
    while (*s1 == *s2) {
        if (*s1 == '\0') {
            return 0;
        }
        s1++;
        s2++;
    }
    return *s1 - *s2;
}