#include <syscalls.h>
#include <ksyscalls.h>
#include <fork.h>

int sys_open (const char *pathname, int flags) {
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

int sys_close(int fd) {
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

long sys_read(unsigned fd, char *buf, size_t count) {
  long ret;
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

long sys_write(unsigned fd, const char *buf, size_t count) {
  long ret;
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

long sys_getpid(void) {
  long ret;
  asm volatile("li a7, %1\n\t"
               "ecall\n\t"
               "mv %0, a0\n\t"
               : "=r"(ret)
               : "i"(__NR_getpid)
               : "a0", "a7", "memory");
  return ret;
}

long sys_clone(struct pt_regs *regs) {
  return do_fork(regs);
}