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

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define DEEPGREEN "\033[36m"
#define CLEAR "\033[0m"

extern FILE __iob[3];

int vfprintf(FILE *restrict f, const char *restrict fmt, va_list ap);

int printf(const char *restrict fmt, ...);

char getchar(void);

uint64_t getcharn(char *buf, uint64_t count);

#endif
