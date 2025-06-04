#include <malloc.h>

int is_free_block(struct malloc_blk *blk) {
    // printf("[U] is_free_block: checking block at %p\n", blk);
    // for (int i = 0; i < BLK_META_SIZE; i++) {
    //     printf("%02x ", ((uint8_t *)&blk->meta)[i]);
    // }
    // printf("\n");
    return blk->meta.magic == MALLOC_MAGIC && blk->meta.free;
}

void* malloc(size_t size) {
    struct malloc_blk *curr_blk = (struct malloc_blk *)USER_HEAP;
    // printf("%d\n", curr_blk->meta.magic);

    if(!(curr_blk->meta.magic == MALLOC_MAGIC)) {
        // printf("[U] malloc: initializing user heap\n");
        for (int i = 0; i < USER_HEAP_SIZE / BLK_SIZE; i++) {
            curr_blk->meta.magic = MALLOC_MAGIC;
            curr_blk->meta.size = 0;
            curr_blk->meta.free = 1;
            curr_blk++;
        }
    }

    curr_blk = (struct malloc_blk *)USER_HEAP;
    int free_needed = ((size + BLK_META_SIZE) / BLK_SIZE) + 1; // round up to nearest block size
    int free_begin = -1;
    for (int i = 0; i < USER_HEAP_SIZE / BLK_SIZE; i++) {
        if (is_free_block(curr_blk)) {
            if (free_begin == -1) {
                free_begin = i;
            }
            if (i - free_begin + 1 >= free_needed) {
                curr_blk -= (free_needed - 1); // move back to the start of the free block 
                curr_blk->meta.free = 0;
                curr_blk->meta.size = size;
                // printf("[U] malloc: allocated %zu bytes at block %d\n", size, free_begin);
                return (void *)curr_blk->data;
            }
        } else {
            free_begin = -1;
        }
        curr_blk++;
    }
    // printf("[U] malloc: out of memory\n");
    return NULL; // out of memory
}

void free(void* ptr) {
    if (ptr == NULL) return;

    struct malloc_blk *blk = (struct malloc_blk *)((uint64_t)ptr - BLK_META_SIZE);
    // printf("[U] free: freeing memory at %p\n", blk);
    if (blk->meta.magic != MALLOC_MAGIC) {
        // printf("[U] free: invalid pointer\n");
        return;
    }

    int free_cnts = ((blk->meta.size + BLK_META_SIZE) / BLK_SIZE) + 1; // round up to nearest block size
    for (int i = 0; i < free_cnts; i++) {
        blk->meta.magic = MALLOC_MAGIC;
        blk->meta.free = 1;
        blk->meta.size = 0;
        blk++;
    }
    // printf("[U] free: memory freed successfully\n");
    return;
}