#ifndef __UNISTD_H__
#define __UNISTD_H__

#include <stddef.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_READ  0x1 // Read permission
#define F_WRITE 0x2 // Write permission
#define F_EXEC  0x4 // Execute permission

typedef int pid_t;
typedef long ssize_t;

typedef unsigned int off_t;

int fopen(const char *pathname, int flags);
int fclose(int fd);
ssize_t fread(int fd, void *buf, size_t count, off_t offset);
ssize_t fwrite(int fd, const void *buf, size_t count, off_t offset);

ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
pid_t getpid(void);
pid_t fork(void);

#endif
