#include <virtio.h>
#include <mm.h>
#include <string.h>

struct virtq virtio_blk_ring;

char virtio_blk_status = 3; // Status of the last request
struct virtio_blk_req virtio_blk_req;

uint64_t virtio_seek_device()
{
    uint64_t virtio_base = VIRTIO_MMIO_START + VIRT_VM_OFFSET;
    uint64_t virtio_end = VIRTIO_MMIO_END + VIRT_VM_OFFSET;
    uint64_t virtio_stride = VIRTIO_MMIO_STRIDE;

    for (uint64_t addr = virtio_base; addr < virtio_end; addr += virtio_stride)
    {
        uint32_t magic_value = *(volatile uint32_t *)(addr + VIRTIO_MMIO_MAGIC_VALUE);
        uint32_t device_id = *(volatile uint32_t *)(addr + VIRTIO_MMIO_DEVICE_ID);
        // printk("Checking VirtIO device at address 0x%lx: magic_value = 0x%x, device_id = 0x%x\n", addr, magic_value, device_id);
        if (magic_value == 0x74726976 && device_id == VIRTIO_DEVICR_ID_BLK)
        {
            printk("Found VirtIO device at address 0x%lx\n", addr);
            return addr; // Found the VirtIO device
        }
    }

    printk("No VirtIO device found\n");
    return 0; // No device found
}

void virtio_init(uint64_t virtio_base)
{
    virtio_blk_driver_init(virtio_base);
    virtio_blk_feature_init(virtio_base);
    virtio_blk_config_init(virtio_base);
    virtio_blk_queue_init(virtio_base);

    char buf[VIRTIO_BLK_SECTOR_SIZE];
    memset(buf, 0xac, VIRTIO_BLK_SECTOR_SIZE);
    virtio_blk_read_sector(virtio_base, 0x0, buf);
    if (buf[510] != 0x55 || buf[511] != 0xaa)
    {
        printk("virtio_blk_init: sector 0 is not a valid MBR\n");
    }
    else
    {
        printk("virtio_blk_init: sector 0 is a valid MBR\n");
    }
    printk("...virtio_blk_init done!\n");
}

void virtio_blk_driver_init(uint64_t virtio_base)
{
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_STATUS) = DEVICE_ACKNOWLEDGE;

    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_STATUS) |= DEVICE_DRIVER;

    asm volatile("fence rw, rw");
    return;
}

void virtio_blk_feature_init(uint64_t virtio_base)
{
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_DEVICE_FEATURES_SEL) = 0;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_DEVICE_FEATURES_SEL) = 1;

    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_DRIVER_FEATURES_SEL) = 0;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_DRIVER_FEATURES) = 0x30000200;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_DRIVER_FEATURES_SEL) = 1;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_DRIVER_FEATURES) = 0x0;

    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_STATUS) |= DEVICE_FEATURES_OK;

    asm volatile("fence rw, rw");
    return;
}

void virtio_blk_config_init(virtio_base)
{
    return;
}

void virtio_blk_queue_init(uint64_t virtio_base)
{
    uint64_t queue_pages = (uint64_t)alloc_pages(3);
    printk("virtio_blk_queue_init: queue_pages = 0x%lx\n", queue_pages);

    virtio_blk_ring.desc = (struct virtq_desc *)queue_pages;
    virtio_blk_ring.avail = (struct virtq_avail *)(queue_pages + PGSIZE);
    virtio_blk_ring.used = (struct virtq_used *)(queue_pages + 2 * PGSIZE);
    virtio_blk_ring.avail->flags = VIRTQ_AVAIL_F_NO_INTERRUPT;

    for (uint16_t i = 0; i < VIRTIO_QUEUE_SIZE; i++)
    {
        virtio_blk_ring.desc[i].next = (i + 1) % VIRTIO_QUEUE_SIZE;
    }

    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_SEL) = 0;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_NUM_MAX) = VIRTIO_QUEUE_SIZE;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_NUM) = VIRTIO_QUEUE_SIZE;

    uint32_t desc_addr_low = (uint32_t)(((uint64_t)virtio_blk_ring.desc - PA2VA_OFFSET) & 0xFFFFFFFF);
    uint32_t desc_addr_high = (uint32_t)(((uint64_t)virtio_blk_ring.desc - PA2VA_OFFSET) >> 32);
    uint32_t avail_addr_low = (uint32_t)(((uint64_t)virtio_blk_ring.avail - PA2VA_OFFSET) & 0xFFFFFFFF);
    uint32_t avail_addr_high = (uint32_t)(((uint64_t)virtio_blk_ring.avail - PA2VA_OFFSET) >> 32);
    uint32_t used_addr_low = (uint32_t)(((uint64_t)virtio_blk_ring.used - PA2VA_OFFSET) & 0xFFFFFFFF);
    uint32_t used_addr_high = (uint32_t)(((uint64_t)virtio_blk_ring.used - PA2VA_OFFSET) >> 32);
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_DESC_LOW) = desc_addr_low;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_DESC_HIGH) = desc_addr_high;
    printk("virtio_blk_queue_init: desc_addr_low = 0x%x, desc_addr_high = 0x%x\n", desc_addr_low, desc_addr_high);
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_AVAIL_LOW) = avail_addr_low;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_AVAIL_HIGH) = avail_addr_high;
    printk("virtio_blk_queue_init: avail_addr_low = 0x%x, avail_addr_high = 0x%x\n", avail_addr_low, avail_addr_high);
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_USED_LOW) = used_addr_low;
    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_USED_HIGH) = used_addr_high;
    printk("virtio_blk_queue_init: used_addr_low = 0x%x, used_addr_high = 0x%x\n", used_addr_low, used_addr_high);
    asm volatile("fence rw, rw");

    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_READY) = 1;
    asm volatile("fence rw, rw");
    return;
}

void virtio_blk_cmd(uint64_t virtio_base, uint32_t type, uint32_t sector, void *buf)
{
    virtio_blk_req.type = type;
    virtio_blk_req.sector = sector;

    uint16_t flags = VIRTQ_DESC_F_NEXT;
    if (type == VIRTIO_BLK_T_IN)
    {
        flags |= VIRTQ_DESC_F_WRITE;
    }

    // request header
    virtio_blk_ring.desc[0].addr = (uint64_t)(&virtio_blk_req) - PA2VA_OFFSET;
    virtio_blk_ring.desc[0].len = sizeof(struct virtio_blk_req);
    virtio_blk_ring.desc[0].flags = VIRTQ_DESC_F_NEXT;
    virtio_blk_ring.desc[0].next = 1;

    if (type == VIRTIO_BLK_T_IN)
    {
    //data buffer
    virtio_blk_ring.desc[1].addr = (uint64_t)buf - PA2VA_OFFSET;
    virtio_blk_ring.desc[1].len = VIRTIO_BLK_SECTOR_SIZE;
    virtio_blk_ring.desc[1].flags = flags;
    virtio_blk_ring.desc[1].next = 2;

    // status buffer
    virtio_blk_ring.desc[2].addr = (uint64_t)(&virtio_blk_status) - PA2VA_OFFSET;
    virtio_blk_ring.desc[2].len = sizeof(virtio_blk_status);
    virtio_blk_ring.desc[2].flags = VIRTQ_DESC_F_WRITE;
    }
    else if (type == VIRTIO_BLK_T_OUT)
    {
        // status buffer
        virtio_blk_ring.desc[1].addr = (uint64_t)(&virtio_blk_status) - PA2VA_OFFSET;
        virtio_blk_ring.desc[1].len = sizeof(virtio_blk_status);
        virtio_blk_ring.desc[1].flags = VIRTQ_DESC_F_WRITE;
    }


    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_STATUS) |= DEVICE_DRIVER_OK; // Indicate that the driver is ready

    virtio_blk_ring.avail->ring[virtio_blk_ring.avail->idx % VIRTIO_QUEUE_SIZE] = 0; // Add the first descriptor index
    virtio_blk_ring.avail->idx++;

    *(volatile uint32_t *)(virtio_base + VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // Notify the device about the new request
    asm volatile("fence rw, rw");


    //printk("virtio_blk_cmd: type = %u, sector = %u, buf = %p\n", type, sector, buf);
    return;
}

void virtio_blk_read_sector(uint64_t virtio_base, uint32_t sector, void *buf)
{
    uint16_t old_idx = virtio_blk_ring.used->idx;
    virtio_blk_cmd(virtio_base, VIRTIO_BLK_T_IN, sector, buf);
    printk("reading[#");
    while (virtio_blk_ring.used->idx == old_idx)
    {
        // Wait for the device to process the request
        printk("=");
    }
    printk("#]\n");
    if (virtio_blk_status != VIRTIO_BLK_S_OK)
    {
        printk("virtio_blk_read_sector: sector %u read failed with status %d\n", sector, virtio_blk_status);
    }
    printk("virtio_blk_read_sector: sector %u read completed successfully\n", sector);
}

void virtio_blk_write_sector(uint64_t virtio_base, uint32_t sector, const void *buf)
{
    memcpy(virtio_blk_req.data, buf, VIRTIO_BLK_SECTOR_SIZE);
    uint16_t old_idx = virtio_blk_ring.used->idx;
    virtio_blk_cmd(virtio_base, VIRTIO_BLK_T_OUT, sector, (void *)buf);
    printk("writing[#");
    while (virtio_blk_ring.used->idx == old_idx)
    {
        // Wait for the device to process the request
        printk("=");
    }
    printk("#]\n");
    if (virtio_blk_status != VIRTIO_BLK_S_OK)
    {
        printk("virtio_blk_write_sector: sector %u write failed with status %d\n", sector, virtio_blk_status);
    }
    printk("virtio_blk_write_sector: sector %u write completed successfully\n", sector);
}