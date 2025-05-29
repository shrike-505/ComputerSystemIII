#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <fs.h>

void fat32_init(uint32_t lba, uint32_t sector_size);

int64_t fat32_lseek(struct fat32_file *file, int64_t offset, uint64_t whence);
int fat32_read_file(struct fat32_file *file, void *buf, uint64_t len);
int fat32_write_file(struct fat32_file *file, const void *buf, uint64_t len);
int fat32_open_file(struct fat32_file *file, const char *path, int flags);
int fat32_create_file(const char *path, int flags);


#endif // FAT32_H