#ifndef VIRTIO_H
#define VIRTIO_H

#include <stdint.h>

#define VIRTIO_MMIO_MAGIC_VALUE   0x000
#define VIRTIO_MMIO_VERSION       0x004
#define VIRTIO_MMIO_DEVICE_ID     0x008
#define VIRTIO_MMIO_VENDOR_ID     0x00C

#define VIRTIO_MMIO_DEVICE_FEATURES 0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES 0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024

#define VIRTIO_MMIO_QUEUE_SEL     0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM     0x038
#define VIRTIO_MMIO_QUEUE_READY   0x044
#define VIRTIO_MMIO_QUEUE_NOTIFY  0x050

#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064

#define VIRTIO_MMIO_STATUS        0x070

#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW 0x0A0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0x0A4

#define VIRTIO_MMIO_CONFIG_GENERATION 0x0FC
#define VIRTIO_MMIO_CONFIG 0x100

#define VIRTIO_MMIO_START         0x10001000
#define VIRTIO_MMIO_END           0x10009000
#define VIRTIO_MMIO_STRIDE        0x1000

#define VIRT_VM_START 0xffffffc000000000
#define VIRT_VM_OFFSET (VIRT_VM_START - VIRTIO_MMIO_START)

#define DEVICE_ACKNOWLEDGE 1
#define DEVICE_DRIVER 2
#define DEVICE_DRIVER_OK 4
#define DEVICE_FEATURES_OK 8
#define DEVICE_NEEDS_RESET 64
#define DEVICE_FAILED 128

#define VIRTIO_DEVICR_ID_BLK  2

#define VIRTIO_QUEUE_SIZE 16

struct virtq_desc { 
        /* Address (guest-physical). */ 
        uint64_t addr; 
        /* Length. */ 
        uint32_t len; 

/* This marks a buffer as continuing via the next field. */ 
#define VIRTQ_DESC_F_NEXT   1 
/* This marks a buffer as device write-only (otherwise device read-only). */ 
#define VIRTQ_DESC_F_WRITE     2 
/* This means the buffer contains a list of buffer descriptors. */ 
#define VIRTQ_DESC_F_INDIRECT   4  
        /* The flags as indicated above. */ 
        uint16_t flags; 
        /* Next field if flags & NEXT */ 
        uint16_t next; 
};

struct virtq_avail { 
#define VIRTQ_AVAIL_F_NO_INTERRUPT      1 
        uint16_t flags; 
        uint16_t idx; 
        uint16_t ring[ VIRTIO_QUEUE_SIZE ]; 
        uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */ 
};

/* uint32_t is used here for ids for padding reasons. */ 
struct virtq_used_elem { 
        /* Index of start of used descriptor chain. */ 
        uint32_t id; 
        /* 
         * The number of bytes written into the device writable portion of 
         * the buffer described by the descriptor chain. 
         */ 
        uint32_t len; 
};

struct virtq_used { 
#define VIRTQ_USED_F_NO_NOTIFY  1 
        uint16_t flags; 
        uint16_t idx; 
        struct virtq_used_elem ring[ VIRTIO_QUEUE_SIZE]; 
        uint16_t avail_event; /* Only if VIRTIO_F_EVENT_IDX */ 
}; 

struct virtq { 
        // The actual descriptors (16 bytes each) 
        struct virtq_desc *desc; 
 
        // A ring of available descriptor heads with free-running index. 
        struct virtq_avail *avail; 
 
        // A ring of used descriptor heads with free-running index. 
        struct virtq_used *used; 
};

#define VIRTIO_BLK_T_IN           0 
#define VIRTIO_BLK_T_OUT          1 
#define VIRTIO_BLK_T_FLUSH        4 
#define VIRTIO_BLK_T_GET_ID       8 
#define VIRTIO_BLK_T_GET_LIFETIME 10 
#define VIRTIO_BLK_T_DISCARD      11 
#define VIRTIO_BLK_T_WRITE_ZEROES 13 
#define VIRTIO_BLK_T_SECURE_ERASE   14

#define VIRTIO_BLK_S_OK        0 
#define VIRTIO_BLK_S_IOERR     1 
#define VIRTIO_BLK_S_UNSUPP    2

#define VIRTIO_BLK_SECTOR_SIZE 512

struct virtio_blk_req { 
        uint32_t type; 
        uint32_t reserved; 
        uint64_t sector; 
        uint8_t data[VIRTIO_BLK_SECTOR_SIZE]; 
        //uint8_t status; 
};

uint64_t virtio_seek_device(); 

void virtio_init(uint64_t virtio_base);

void virtio_blk_cmd(uint64_t virtio_base, uint32_t type, uint32_t sector, void* buf);

void virtio_blk_read_sector(uint64_t virtio_base, uint32_t sector, void* buf);

void virtio_blk_write_sector(uint64_t virtio_base, uint32_t sector, const void* buf);

#endif // VIRTIO_H