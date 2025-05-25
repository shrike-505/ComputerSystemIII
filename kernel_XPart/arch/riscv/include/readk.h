#ifndef _READK_H
#define _READK_H

#include <sbi.h>
#include <stdio.h>
#include <private_kdefs.h>

struct sbiret sbi_read(uint64_t num_bytes, void* buf);

char getchark();

uint64_t readk(char* buf, uint64_t count);

#endif // _READK_H