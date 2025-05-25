#ifndef __STDIO_H__
#define __STDIO_H__

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

typedef struct FILE {
  int (*write)(struct FILE *, const void *, size_t);
} FILE;

#define stdin (&__iob[0])
#define stdout (&__iob[1])
#define stderr (&__iob[2])

extern FILE __iob[3];

int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);

int printf(const char *restrict fmt, ...);

char getchar(void);

uint64_t getcharn(char *buf, uint64_t count);

#endif
