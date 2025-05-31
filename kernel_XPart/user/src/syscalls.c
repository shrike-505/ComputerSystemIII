#include <syscalls.h>
#include <unistd.h>
#include <stdint.h>

pid_t getpid(void) {
  pid_t ret;
  asm volatile("li a7, %1\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_getpid)
               : "a0", "a7", "memory");
  return ret;
}

int fopen(const char *pathname, int flags) {
  int fd;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "mv a1, %3\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(fd)
               : "i"(__NR_open), "r"(pathname), "r"(flags)
               : "a0", "a1", "a7", "memory");
  return fd;
}

int fclose(int fd) {
  int ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_close), "r"(fd)
               : "a0", "a7", "memory");
  return ret;
}

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

ssize_t lseek( int fd, off_t offset, int whence) {
  ssize_t ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "mv a1, %3\n\t"
               "mv a2, %4\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_lseek), "r"(fd), "r"(offset), "r"(whence)
               : "a0", "a1", "a2", "a7", "memory");
  return ret;
}

ssize_t read(int fd, void *buf, size_t count) {
  ssize_t ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "mv a1, %3\n\t"
               "mv a2, %4\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_read), "r"(fd), "r"(buf), "r"(count)
               : "a0", "a1", "a2", "a7", "memory");
  return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
  ssize_t ret;
  asm volatile("li a7, %1\n\t"
               "mv a0, %2\n\t"
               "mv a1, %3\n\t"
               "mv a2, %4\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_write), "r"(fd), "r"(buf), "r"(count)
               : "a0", "a1", "a2", "a7", "memory");
  return ret;
}

ssize_t fread(int fd, void *buf, size_t count, off_t offset) {
  off_t pos = lseek(fd, offset, SEEK_SET);
  if (pos < 0) {
    return -1; // lseek failed
  }
  ssize_t bytes_read = read(fd, buf, count);
  if (bytes_read < 0) {
    return -1; // read failed
  }
  return bytes_read; // return the number of bytes read
}

ssize_t fwrite(int fd, const void *buf, size_t count, off_t offset) {
  off_t pos = lseek(fd, offset, SEEK_SET);
  if (pos < 0) {
    return -1; // lseek failed
  }
  ssize_t bytes_written = write(fd, buf, count);
  if (bytes_written < 0) {
    return -1; // write failed
  }
  return bytes_written; // return the number of bytes written
}

pid_t fork(void) {
  pid_t ret;
  asm volatile("li a7, %1\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_clone)
               : "a0", "a7", "memory");
  return ret;
}