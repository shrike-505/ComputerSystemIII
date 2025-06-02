#include <fs.h>
#include <string.h>
#include <printk.h>
#include <readk.h>

struct files_struct files;
uint64_t avail_file_desc = 3; // 0, 1, 2 are reserved for stdin, stdout, stderr 

uint64_t virtio_base;

void file_init_fds()
{
    //stdin
    files.fd_array[0].opened = 1;
    files.fd_array[0].perms = F_READ | F_WRITE; 
    files.fd_array[0].cfo = 0;
    files.fd_array[0].fs_type = FS_TYPE_UNSUPPORTED; 
    files.fd_array[0].lock_pid = SHARED;
    files.fd_array[0].lseek = NULL;
    files.fd_array[0].write = NULL;
    files.fd_array[0].read = getcharnk_sbi_read_fileio;
    files.fd_array[0].path[0] = '\0';

    //stdout and stderr
    for (int i = 1; i < 3; i++)
    {
        files.fd_array[i].opened = 1;
        files.fd_array[i].perms = F_READ | F_WRITE; 
        files.fd_array[i].cfo = 0;
        files.fd_array[i].fs_type = FS_TYPE_UNSUPPORTED; 
        files.fd_array[i].lock_pid = SHARED;
        files.fd_array[i].lseek = NULL;
        files.fd_array[i].write = printk_sbi_write_fileio;
        files.fd_array[i].read = NULL;
        files.fd_array[i].path[0] = '\0';
    }

    // other
    for (int i = 3; i < MAX_FILE_NUMBER; i++)
    {
        files.fd_array[i].opened = 0;
        files.fd_array[i].perms = 0;
        files.fd_array[i].cfo = 0;
        files.fd_array[i].fs_type = FS_TYPE_UNSUPPORTED;
        files.fd_array[i].lock_pid = UNLOCKED;
        files.fd_array[i].lseek = file_lseek;
        files.fd_array[i].write = file_write;
        files.fd_array[i].read = file_read;
        files.fd_array[i].path[0] = '\0';
    }
}

void file_init()
{
    virtio_base = virtio_seek_device();
    printk("virtio base = 0x%lx\n", virtio_base);
    virtio_init(virtio_base);

    fat32_init(virtio_base);

    file_init_fds();

    // fat32_create_file(virtio_base, "hello.elf");
    // int fd = file_open(&files.fd_array[avail_file_desc], "hello.elf", F_READ | F_WRITE);
    // char* content = "";
    // file_write(&files.fd_array[fd], content, 47265);
    // file_close(&files.fd_array[fd]);

    printk("...file_system_init done!\n");
}

int32_t file_open(struct file *file, const char *path, int flags)
{
    if (file == NULL || path == NULL || flags <= 0 || flags > (F_READ | F_WRITE | F_EXEC))
    {
        return -1; // Invalid parameters
    }
    if (file->opened)
    {
        return -1; // File already opened
    }
    int res = fat32_open_file(virtio_base, &file->fat32_file, path);
    if (res < 0)
    {
        return -1; // Failed to open file
    }
    file->opened = 1;
    file->perms = flags;
    file->cfo = 0; // Current file offset
    file->fs_type = FS_TYPE_FAT32; // Set file system type
    file->lock_pid = UNLOCKED; // File is unlocked
    file->lseek = file_lseek;
    file->write = file_write;
    file->read = file_read;
    memcpy(file->path, path, MAX_PATH_LENGTH - 1);
    file->path[MAX_PATH_LENGTH - 1] = '\0'; // Ensure null termination
    printk("File opened: %s, fs_type = %d\n", file->path, file->fs_type);
    return avail_file_desc; // Success
}

int32_t file_close(struct file *file)
{
    if (file == NULL || !file->opened)
    {
        return -1; // Invalid file or file not opened
    }
    file->opened = 0;
    file->perms = 0;
    file->cfo = 0;
    file->fs_type = FS_TYPE_UNSUPPORTED; // Reset file system type
    file->lock_pid = UNLOCKED; // File is unlocked
    file->lseek = NULL;
    file->write = NULL;
    file->read = NULL;
    printk("File closed: %s\n", file->path);
    file->path[0] = '\0'; // Clear path
    return 0; // Success
}

int64_t file_lseek(struct file *file, int64_t offset, uint64_t whence)
{
    if (file == NULL || !file->opened || (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END))
    {
        return -1; // Invalid parameters
    }
    
    int64_t new_offset = file->cfo;

    switch (whence)
    {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset += offset;
            break;
        case SEEK_END:
            new_offset = file->fat32_file.size + offset; // Assuming size is the end of the file
            break;
        default:
            return -1; // Invalid whence
    }

    if (new_offset < 0)
    {
        return -1; // Out of bounds
    }

    file->cfo = new_offset;
    return new_offset; // Return the new file offset
}

int64_t file_write(struct file *file, const void *buf, uint64_t len)
{
    if (file == NULL || !file->opened || !(file->perms & F_WRITE) || buf == NULL || len == 0)
    {
        printk("file_write: Invalid parameters\n");
        printk("file_write: file = %p, opened = %d, perms = %d, buf = %p, len = %lu\n", 
               file, file->opened, file->perms, buf, len);
        return -1; // Invalid parameters
    }
    
    if (file->fs_type != FS_TYPE_FAT32)
    {
        printk("file_write: Unsupported file system type %d\n", file->fs_type);
        return -1; // Unsupported file system type
    }

    int res = fat32_write_file(virtio_base, &file->fat32_file, buf, len, file->cfo);
    if (res < 0)
    {
        printk("file_write: Write failed with error %d\n", res);
        return -1; // Write failed
    }

    file->cfo += res; // Update current file offset
    return res; // Return number of bytes written
}

int64_t file_read(struct file *file, void *buf, uint64_t len)
{
    if (file == NULL || !file->opened || !(file->perms & F_READ) || buf == NULL || len == 0)
    {
        return -1; // Invalid parameters
    }
    
    if (file->fs_type != FS_TYPE_FAT32)
    {
        return -1; // Unsupported file system type
    }

    int res = fat32_read_file(virtio_base, &file->fat32_file, buf, len);
    if (res < 0)
    {
        return -1; // Read failed
    }

    file->cfo += res; // Update current file offset
    return res; // Return number of bytes read
}