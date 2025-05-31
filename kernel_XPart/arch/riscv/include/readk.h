#ifndef _READK_H
#define _READK_H

#include <sbi.h>
#include <stdio.h>
#include <stdint.h>
#include <private_kdefs.h>
#include <fs.h>

struct sbiret sbi_read(uint64_t num_bytes, void* buf);

char getchark();

uint64_t readk(char* buf, uint64_t count);

int64_t getcharnk_sbi_read_fileio(struct file *file, void *buf, uint64_t len);

#endif // _READK_H