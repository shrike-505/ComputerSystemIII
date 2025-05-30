#include <mbr.h>
#include <virtio.h>
#include <string.h>
#include <printk.h>

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

