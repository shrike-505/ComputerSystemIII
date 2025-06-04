#ifndef MALLOC_H
#define MALLOC_H

#include <stdint.h>
#include <stddef.h>

#define USER_HEAP 0x100000 // user heap start virtual address
#define USER_HEAP_SIZE 0x10000 // user heap size

#define MALLOC_MAGIC 0x72696b65

struct malloc_blk_meta
{
    uint32_t magic;
    uint32_t size;
    uint8_t free;
    uint8_t padding[7]; // 16-byte alignment
};

struct malloc_blk
{
    struct malloc_blk_meta meta;
    uint8_t data[240]; 
};

#define BLK_META_SIZE 0x10 // 16 bytes for metadata
#define BLK_SIZE 0x100 // 256 bytes per block

void* malloc(size_t size);
void free(void* ptr);


#endif // MALLOC_H