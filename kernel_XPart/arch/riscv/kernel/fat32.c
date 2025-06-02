#include <fat32.h>
#include <mbr.h>
#include <printk.h>
#include <stdio.h>
#include <string.h>
#include <virtio.h>
#include <string.h>

struct fat32_bpb fat32_bpb; // BIOS Parameter Block for FAT32

struct fat32_volume fat32_volume; // FAT32 volume information

char fat32_table[VIRTIO_BLK_SECTOR_SIZE]; // Buffer for FAT32 table

uint32_t get_next_fat32_cluster(uint64_t virtio_base, uint32_t cluster)
{
    if (cluster < 2 || cluster >= (fat32_bpb.tot_sec32 / fat32_volume.sec_per_cluster))
    {
        printk("get_next_fat32_cluster: Invalid cluster number %u\n", cluster);
        return 0xFFFFFFFF; // Invalid cluster
    }
    uint32_t fat_offset = cluster * 4; // Each FAT32 entry is 4 bytes
    uint32_t fat_sector = fat_offset / VIRTIO_BLK_SECTOR_SIZE;
    uint32_t fat_entry_offset = fat_offset % VIRTIO_BLK_SECTOR_SIZE;
    
    // Get the next cluster from the FAT table
    uint32_t next_cluster = *(uint32_t *)(fat32_table + fat_entry_offset);
    
    return next_cluster & 0x0FFFFFFF; // Mask to get the valid cluster number
}

uint32_t allocate_fat32_cluster(uint64_t virtio_base, uint32_t last_cluster)
{
    // Find the first free cluster in the FAT32 table
    for (uint32_t i = 2; i < (fat32_bpb.tot_sec32 / fat32_volume.sec_per_cluster); i++)
    {
        uint32_t ith_fat_offset = i * 4; 
        uint32_t ith_fat_sector = ith_fat_offset / VIRTIO_BLK_SECTOR_SIZE;
        uint32_t ith_fat_entry_offset = ith_fat_offset % VIRTIO_BLK_SECTOR_SIZE;
        uint32_t ith_entry_value = *(uint32_t *)(fat32_table + ith_fat_entry_offset);
        if (ith_entry_value == 0)
        {
            *(uint32_t *)(fat32_table + ith_fat_entry_offset) = 0x0FFFFFF8; // Mark as end of file
            // Point last->next to this new cluster
            if (last_cluster != 0)
            {
                uint32_t last_fat_offset = last_cluster * 4;
                uint32_t last_fat_sector = last_fat_offset / VIRTIO_BLK_SECTOR_SIZE;
                uint32_t last_fat_entry_offset = last_fat_offset % VIRTIO_BLK_SECTOR_SIZE;
                *(uint32_t *)(fat32_table + last_fat_entry_offset) = i & 0x0FFFFFFF;
            }
            virtio_blk_write_sector(virtio_base, fat32_volume.first_fat_sec, fat32_table); // Write the updated FAT32 table back to disk
            printk("allocate_fat32_cluster: Allocated cluster %u\n", i);
            return i & 0x0FFFFFFF;; // Return the allocated cluster number
        }
    }
    printk("allocate_fat32_cluster: No free clusters available\n");
    return 0xFFFFFFFF; // No free clusters available
}

void fat32_init(uint64_t virtio_base)
{
    struct mbr mbr;
    mbr_read(virtio_base, &mbr);
    for (int i = 0; i < 4; i++)
    {
        if (mbr.partitions[i].type != 0x83)
        {
            continue; // Skip non-Linux partitions
        }
        uint32_t lba_first = mbr.partitions[i].lba_first;
        uint32_t size = mbr.partitions[i].size;
        fat32_init_sector(virtio_base, lba_first, size);
        break;
    }
    printk("...fat32_init done!\n");
}

void fat32_init_sector(uint64_t virtio_base, uint32_t lba, uint32_t sector_size)
{
    virtio_blk_read_sector(virtio_base, lba, &fat32_bpb);
    // printk_blk_rawdata((const char *)&fat32_bpb);
    if (fat32_bpb.boot_sector_signature != 0xAA55)
    {
        printk("fat32_init_sector: Invalid boot sector signature\n");
        return;
    }
    if (fat32_bpb.fil_sys_type[0] != 'F' || fat32_bpb.fil_sys_type[1] != 'A' || fat32_bpb.fil_sys_type[2] != 'T' || fat32_bpb.fil_sys_type[3] != '3' || fat32_bpb.fil_sys_type[4] != '2')
    {
        printk("fat32_init_sector: Not a FAT32 filesystem\n");
        return;
    }
    fat32_volume.first_data_sec = lba + fat32_bpb.rsvd_sec_cnt + fat32_bpb.num_fats * fat32_bpb.fat_sz32;
    fat32_volume.first_fat_sec = lba + fat32_bpb.rsvd_sec_cnt;
    fat32_volume.sec_per_cluster = fat32_bpb.sec_per_clus;
    fat32_volume.fat_sz = fat32_bpb.fat_sz32;
    printk("fat32_init_sector: first_data_sec = %lu, first_fat_sec = %lu, sec_per_cluster = %u, fat_sz = %u\n",
           fat32_volume.first_data_sec, fat32_volume.first_fat_sec, fat32_volume.sec_per_cluster, fat32_volume.fat_sz);

    // Read the FAT32 table
    virtio_blk_read_sector(virtio_base, fat32_volume.first_fat_sec, fat32_table);
}

void get_fat32_filenames(uint64_t virtio_base, uint64_t *filenames) // uint64_t == char[8]
{
    uint32_t cluster = fat32_bpb.root_clus;
    uint32_t sector = fat32_volume.first_data_sec + (cluster - 2) * fat32_volume.sec_per_cluster;
    char rootdir_entry_buf[VIRTIO_BLK_SECTOR_SIZE];
    int cnt = 0;

    for (int i = 0; i < fat32_volume.sec_per_cluster; i++)
    {
        virtio_blk_read_sector(virtio_base, sector + i, rootdir_entry_buf);
        // printk_blk_rawdata((const char *)rootdir_entry_buf);
        for (int j = 0; j < VIRTIO_BLK_SECTOR_SIZE / sizeof(struct fat32_dir_entry); j++)
        {
            struct fat32_dir_entry *dir_entry = (struct fat32_dir_entry *)(rootdir_entry_buf + j * sizeof(struct fat32_dir_entry));
            if (dir_entry->name[0] == 0x00)
            {
                return; // End of directory entries
            }
            if (dir_entry->name[0] == 0xE5 || dir_entry->attr & 0x08)
            {
                continue; // Skip deleted or long name entries
            }
            printk("Filename: %.8s.%.3s, Size: %u bytes\n", dir_entry->name, dir_entry->ext, dir_entry->size);
            memcpy((void *)(filenames + cnt), dir_entry->name, 8);
        }
    }
}

void to_upper_case(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] >= 'a' && str[i] <= 'z')
        {
            str[i] -= 32;
        }
    }
}

void to_fat32_name_format(char *name) // abc.xy -> ABC     XY
{
    to_upper_case(name);
    int i = 0;
    for (; i < 8 && name[i] != '.' && name[i] != '\0'; i++)
        ;
    int dot_pos = i;
    char ext[3];
    int j = 0;
    for (; j < 3 && name[dot_pos + 1 + j] != '\0'; j++)
        ext[j] = name[dot_pos + 1 + j];
    for (; j < 3; j++)
        ext[j] = ' ';
    for (; i < 8; i++)
        name[i] = ' ';
    for (j = 0; j < 3; j++)
        name[8 + j] = ext[j];
}

int fat32_open_file(uint64_t virtio_base, struct fat32_file *file, const char *path)
{
    if (file == NULL || path == NULL)
    {
        return -1; // Invalid parameters
    }

    char name[12]; // 8.3 format
    memset(name, 0, sizeof(name));
    memcpy(name, path, 11); // Copy up to 11 characters
    to_fat32_name_format(name);
    printk("Opening file: %s\n", name);

    uint32_t cluster = fat32_bpb.root_clus;
    uint32_t sector = fat32_volume.first_data_sec + (cluster - 2) * fat32_volume.sec_per_cluster;
    char rootdir_entry_buf[VIRTIO_BLK_SECTOR_SIZE];

    for (int i = 0; i < fat32_volume.sec_per_cluster; i++)
    {
        virtio_blk_read_sector(virtio_base, sector + i, rootdir_entry_buf);
        for (int j = 0; j < VIRTIO_BLK_SECTOR_SIZE / sizeof(struct fat32_dir_entry); j++)
        {
            struct fat32_dir_entry *dir_entry = (struct fat32_dir_entry *)(rootdir_entry_buf + j * sizeof(struct fat32_dir_entry));
            if (dir_entry->name[0] == 0x00)
            {
                printk("File not found: %s\n", name);
                return -1; // End of directory entries
            }
            if (dir_entry->name[0] == 0xE5 || dir_entry->attr & 0x08)
            {
                continue; // Skip deleted or long name entries
            }
            char *name_cmp = (char *)dir_entry->name;
            int cmp = strncmp(name_cmp, name, 11);
            // printk("Comparing: %.8s.%.3s with %s, cmp result: %d\n", dir_entry->name, dir_entry->ext, name, cmp);
            if (cmp == 0)
            {
                file->cluster = dir_entry->startlow | ((uint32_t)dir_entry->starthi << 16);
                memcpy(file->name, dir_entry->name, 11);
                file->size = dir_entry->size;
                file->dir.cluster = cluster;
                file->dir.index = j;
                printk("File found: %s, Cluster: %u,  Size: %u bytes\n", name, file->cluster, dir_entry->size);
                return 0; // File found
            }
        }
    }

    printk("File not found: %s\n", name);
    return -1; // File not found
}

int fat32_read_file(uint64_t virtio_base, struct fat32_file *file, void *buf, uint64_t len)
{
    if (file == NULL || buf == NULL || len == 0)
    {
        return -1; // Invalid parameters
    }

    uint32_t cluster = file->cluster;
    printk("Reading file from cluster: %u, size: %u bytes\n", cluster, file->size);
    uint32_t size = file->size;
    if (len > size)
    {
        len = size; // Adjust length to file size
    }
    uint64_t bytes_read = 0;
    uint64_t sector = fat32_volume.first_data_sec + (cluster - 2) * fat32_volume.sec_per_cluster;
    printk("Starting sector: %lu\n", sector);
    char sector_buf[VIRTIO_BLK_SECTOR_SIZE];

    while (bytes_read < len)
    {

        virtio_blk_read_sector(virtio_base, sector, sector_buf);
        //printk_blk_rawdata((const char *)sector_buf);
        memcpy((char *)buf + bytes_read, sector_buf, VIRTIO_BLK_SECTOR_SIZE);
        bytes_read += VIRTIO_BLK_SECTOR_SIZE;

        // Get next cluster
        uint32_t next_cluster = get_next_fat32_cluster(virtio_base, cluster);
        printk("Next cluster: %u\n", next_cluster);
        if (cluster >= 0x0FFFFFF8) // End of file marker
        {
            break;
        }
        cluster = next_cluster;
        sector = fat32_volume.first_data_sec + (cluster - 2) * fat32_volume.sec_per_cluster;
        printk("Next sector: %lu\n", sector);
    }
    
    return bytes_read; // Return the number of bytes read
}

int fat32_write_file(uint64_t virtio_base, struct fat32_file *file, const void *buf, uint64_t len, uint64_t offset)
{
    if (file == NULL || buf == NULL || len == 0)
    {
        printk("fat32_write_file: Invalid parameters\n");
        return -1; // Invalid parameters
    }

    uint32_t cluster = file->cluster;
    uint32_t size = file->size;
    if (offset > size)
    {
        printk("fat32_write_file: Offset exceeds file size\n");
        return -1; // Write exceeds file size
    }
    
    uint64_t bytes_written = 0;
    uint64_t sector = fat32_volume.first_data_sec + (cluster - 2) * fat32_volume.sec_per_cluster;
    char sector_buf[VIRTIO_BLK_SECTOR_SIZE];

    uint32_t start_sector = sector + offset / VIRTIO_BLK_SECTOR_SIZE;
    uint32_t start_offset = offset % VIRTIO_BLK_SECTOR_SIZE;
    printk("Writing file from cluster: %u, size: %u bytes, offset: %lu\n", cluster, size, offset);
    //printk("Starting sector: %lu, start_offset: %u\n", start_sector, start_offset);

    while (bytes_written < len)
    {
        virtio_blk_read_sector(virtio_base, start_sector, sector_buf);
        uint64_t bytes_to_write = VIRTIO_BLK_SECTOR_SIZE - start_offset;
        if (start_offset + len - bytes_written < VIRTIO_BLK_SECTOR_SIZE)
        {
            bytes_to_write = len - bytes_written - start_offset; 
        }
        memcpy(sector_buf + start_offset, (const char *)buf + bytes_written, bytes_to_write);
        uint64_t write_end = start_offset + bytes_to_write;
        virtio_blk_write_sector(virtio_base, start_sector, sector_buf);
        bytes_written += bytes_to_write;
        //printk("Wrote %lu bytes to sector %lu, start_offset: %u\n", bytes_to_write, start_sector, start_offset);
        start_offset = 0; // After the first write, start_offset is always 0
        if (bytes_written >= len)
        {
            break; // All bytes written
        }

        // Get next cluster
        uint32_t next_cluster = get_next_fat32_cluster(virtio_base, cluster);
        if (next_cluster >= 0x0FFFFFF8) // End of file marker
        {
            next_cluster = allocate_fat32_cluster(virtio_base, cluster); // Allocate a new cluster if needed
        }
        cluster = next_cluster;
        start_sector = fat32_volume.first_data_sec + (cluster - 2) * fat32_volume.sec_per_cluster;
        //printk("Next cluster: %u, Next sector: %lu\n", cluster, start_sector);
    }

    if (offset + bytes_written > size)
    {
        file->size = offset + bytes_written; // Update file size if it has increased
    }
    printk("Total bytes written: %lu, New file size: %u\n", bytes_written, file->size);
    // Update the directory entry with the new size
    uint32_t rootdir_cluster = fat32_bpb.root_clus;
    uint32_t rootdir_sector = fat32_volume.first_data_sec + (rootdir_cluster - 2) * fat32_volume.sec_per_cluster;
    char rootdir_entry_buf[VIRTIO_BLK_SECTOR_SIZE];
    for (int i = 0; i < fat32_volume.sec_per_cluster; i++)
    {
        virtio_blk_read_sector(virtio_base, rootdir_sector + i, rootdir_entry_buf);
        for (int j = 0; j < VIRTIO_BLK_SECTOR_SIZE / sizeof(struct fat32_dir_entry); j++)
        {
            //printk_blk_rawdata((const char *)rootdir_entry_buf);
            struct fat32_dir_entry *dir_entry = (struct fat32_dir_entry *)(rootdir_entry_buf + j * sizeof(struct fat32_dir_entry));
            if (dir_entry->name[0] == 0x00)
            {
                continue; // End of directory entries
            }
            if (dir_entry->name[0] == 0xE5 || dir_entry->attr & 0x08)
            {
                continue; // Skip deleted or long name entries
            }
            char *name_cmp = (char *)dir_entry->name;
            //printk("Comparing: %.8s.%.3s with %s\n", dir_entry->name, dir_entry->ext, file->name);
            int cmp = strncmp(name_cmp, file->name, 11);
            if (cmp == 0)
            {
                dir_entry->size = file->size; // Update size
                memcpy(rootdir_entry_buf + j * sizeof(struct fat32_dir_entry), dir_entry, sizeof(struct fat32_dir_entry));
                virtio_blk_write_sector(virtio_base, rootdir_sector + i, rootdir_entry_buf); // Write the updated directory entry back to disk
                break;
            }
        }
    }

    return bytes_written; // Return the number of bytes written
}

int fat32_create_file(uint64_t virtio_base, const char *path)
{
    if (path == NULL)
    {
        return -1; // Invalid parameters
    }

    char name[12]; // 8.3 format
    memset(name, 0, sizeof(name));
    memcpy(name, path, 11); // Copy up to 11 characters
    to_fat32_name_format(name);
    printk("Creating file: %s\n", name);

    uint32_t cluster = fat32_bpb.root_clus;
    uint32_t sector = fat32_volume.first_data_sec + (cluster - 2) * fat32_volume.sec_per_cluster;
    char rootdir_entry_buf[VIRTIO_BLK_SECTOR_SIZE];

    for (int i = 0; i < fat32_volume.sec_per_cluster; i++)
    {
        virtio_blk_read_sector(virtio_base, sector + i, rootdir_entry_buf);
        for (int j = 0; j < VIRTIO_BLK_SECTOR_SIZE / sizeof(struct fat32_dir_entry); j++)
        {
            struct fat32_dir_entry *dir_entry = (struct fat32_dir_entry *)(rootdir_entry_buf + j * sizeof(struct fat32_dir_entry));
            if (dir_entry->name[0] == 0x00)
            {
                // Found an empty entry, create the file here
                memcpy(dir_entry->name, name, 11);
                dir_entry->attr = 0x20; // Set as a file
                dir_entry->size = 0; // Initial size is 0
                dir_entry->startlow = allocate_fat32_cluster(virtio_base, 0); // Allocate a new cluster
                dir_entry->starthi = 0;

                memcpy(rootdir_entry_buf + j * sizeof(struct fat32_dir_entry), dir_entry, sizeof(struct fat32_dir_entry));
                virtio_blk_write_sector(virtio_base, sector + i, rootdir_entry_buf); // Write the updated directory entry back to disk
                printk("File created: %s\n", name);
                return 0; // File created successfully
            }
        }
    }

    printk("Failed to create file: %s\n", name);
    return -1; // No space left in the root directory
}
