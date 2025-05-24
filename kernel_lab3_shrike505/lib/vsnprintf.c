#include <inttypes.h>
#include <stdio.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define NL_ARGMAX 9

#ifdef MAX
#undef MAX
#endif
#define MAX(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b;      \
  })

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b)           \
  ({                        \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a < _b ? _a : _b;      \
  })

struct str_file {
    FILE f;
    char *buf;
    size_t size;
    size_t pos;
};

static void str_write(FILE *f, const char *s, size_t l) {
    struct str_file *sf = (struct str_file *)f;
    size_t can_write = sf->pos < sf->size ? sf->size - sf->pos - 1 : 0;
    if (can_write > 0) {
        size_t to_write = MIN(can_write, l);
        memcpy(sf->buf + sf->pos, s, to_write);
    }
    sf->pos += l;
}

int vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    struct str_file sf = {
        .f = {.write = str_write},
        .buf = str,
        .size = size,
        .pos = 0
    };
    
    int ret = vfprintf(&sf.f, format, ap);
    if (size > 0)
        str[MIN(sf.pos, size - 1)] = '\0';
    return ret;
}

int snprintf(char *str, size_t size, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, size, format, ap);
    va_end(ap);
    return ret;
}

int sprintf(char *str, const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(str, SIZE_MAX, format, ap);
    va_end(ap);
    return ret;
}