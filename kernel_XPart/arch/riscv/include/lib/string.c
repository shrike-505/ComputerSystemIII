#include <string.h>

void *memset(void *restrict dst, int c, size_t n) {
    unsigned char *d = dst;
    for (size_t i = 0; i < n; i++) {
        d[i] = c;
    }
    return dst;
}

size_t strnlen(const char *restrict s, size_t maxlen) {
    size_t len = 0;
    while (len < maxlen && s[len]) {
        len++;
    }
    return len;
}
