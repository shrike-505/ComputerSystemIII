#include <sbi.h>
#include <stdio.h>
#include <printk.h>

#define RELEASE 1

size_t printk_sbi_write(FILE *restrict fp, const void *restrict buf, size_t len) {
  (void)fp;
  const char *p = buf;
  for (size_t i = 0; i < len; i++) {
    sbi_ecall(0x4442434e, 2, p[i], 0, 0, 0, 0, 0);
  }
  return len;
}

int64_t printk_sbi_write_fileio(struct file *file, const void *buf, uint64_t len) {
  size_t written = printk_sbi_write(NULL, buf, len);
  return written;
}

void printk(const char *fmt, ...) {
  #ifndef RELEASE
  FILE printk_out = {
      .write = printk_sbi_write,
  };

  va_list ap;
  va_start(ap, fmt);
  vfprintf(&printk_out, fmt, ap);
  va_end(ap);
  #endif
}

void printk_blk_rawdata(const char *buf) {
  #ifdef PRINTK_BLK_RAWDATA
  for (size_t i = 0; i < 512; i++) {
    printk("%02x ", (unsigned char)buf[i]);
    if ((i + 1) % 16 == 0) {
      printk("\n");
    }
  }
  #endif
}