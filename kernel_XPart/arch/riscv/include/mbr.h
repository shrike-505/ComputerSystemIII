#ifndef MBR_H
#define MBR_H

#include <stdint.h>
#include <private_kdefs.h>

struct mbr_partition_entry {
    uint8_t status;
    uint8_t chs_first[3]; // CHS of the first sector
    uint8_t type;         // Partition type
    uint8_t chs_last[3];  // CHS of the last sector
    uint32_t lba_first;   // LBA of the first sector
    uint32_t size;        // Size of the partition in sectors
} __attribute__((packed));

struct mbr {
    uint8_t boot_code[440];                   // Boot code (not used in this implementation)
    uint32_t disk_signature;                  // Disk signature
    uint16_t reserved;                        // Reserved (should be 0)
    struct mbr_partition_entry partitions[4]; // Four partition entries
    uint16_t boot_signature;                  // Boot signature (0x55AA)
} __attribute__((packed));

void mbr_read(uint64_t virtio_base, struct mbr *mbr);
void mbr_print(const struct mbr *mbr);

#endif // MBR_H