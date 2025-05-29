#include <mbr.h>
#include <virtio.h>

void mbr_read(uint64_t virtio_base, struct mbr *mbr)
{
    char buf[VIRTIO_BLK_SECTOR_SIZE];
    virtio_blk_read_sector(virtio_base, 0, buf);
    if (buf[510] != 0x55 || buf[511] != 0xaa)
    {
        printk("mbr_read: sector 0 is not a valid MBR\n");
        return;
    }
    else
    {
        printk("mbr_read: sector 0 is a valid MBR\n");
    }
    memcpy(mbr, buf, sizeof(struct mbr));
}

void mbr_print(const struct mbr *mbr)
{
    printk("MBR Disk Signature: 0x%08x\n", mbr->disk_signature);
    printk("MBR Boot Signature: 0x%04x\n", mbr->boot_signature);
    for (int i = 0; i < 4; i++)
    {
        const struct mbr_partition_entry *entry = &mbr->partitions[i];
        if (entry->type != 0)
        {
            printk("Partition %d:\n", i + 1);
            printk("  Status: 0x%02x\n", entry->status);
            printk("  CHS First: 0x%02x%02x%02x\n", entry->chs_first[0], entry->chs_first[1], entry->chs_first[2]);
            printk("  CHS Last: 0x%02x%02x%02x\n", entry->chs_last[0], entry->chs_last[1], entry->chs_last[2]);
            printk("  Type: 0x%02x\n", entry->type);
            printk("  First Sector (LBA): %u\n", entry->lba_first);
            printk("  Size (sectors): %u\n", entry->size);
        }
    }
} 