#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdint.h>
#include <inttypes.h>
#include <fs.h>

void printk(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

void printk_blk_rawdata(const char *buf);

int64_t printk_sbi_write_fileio(struct file *file, const void *buf, uint64_t len);

#endif
