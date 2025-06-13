#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <virtio.h>
#include <mbr.h>
#include <fat32.h>

#define MAX_FILE_NUMBER 32
#define MAX_PATH_LENGTH 32

struct fat32_dir {
    uint32_t cluster;
    uint32_t index;     // entry index in the cluster
};

struct fat32_file {
    uint32_t cluster;
    char name[11];
    uint32_t size;      // file size in bytes
    struct fat32_dir dir;
};

struct file {   // Opened file in a thread.
    uint32_t opened;
#define F_READ  0x1 // Read permission
#define F_WRITE 0x2 // Write permission
#define F_EXEC  0x4 // Execute permission
    uint32_t perms;
    int64_t cfo;
#define FS_TYPE_FAT32 0x1
#define FS_TYPE_UNSUPPORTED  0x0
    uint32_t fs_type;
#define UNLOCKED -1
#define SHARED 0
    int32_t lock_pid;

    union {
        struct fat32_file fat32_file;
    };

    int64_t (*lseek) (struct file *file, int64_t offset, uint64_t whence);
    int64_t (*write) (struct file *file, const void *buf, uint64_t len);
    int64_t (*read)  (struct file *file, void *buf, uint64_t len);

    char path[MAX_PATH_LENGTH];
};

struct files_struct {
    struct file fd_array[MAX_FILE_NUMBER];
};

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

void file_init();
int32_t file_open(struct file *file, const char *path, int flags);
int32_t file_close(struct file *file);
int64_t file_lseek(struct file *file, int64_t offset, uint64_t whence);
int64_t file_write(struct file *file, const void *buf, uint64_t len);
int64_t file_read(struct file *file, void *buf, uint64_t len);
uint32_t get_fs_type(const char *filename);
char** get_filenames();

#endif // FS_H