#ifndef __KSYSCALLS_H__
#define __KSYSCALLS_H__

#include <stddef.h>
#include <proc.h>

int sys_open(const char *pathname, int flags);
int sys_close(int fd);
long sys_read(unsigned fd, char *buf, size_t count);
long sys_write(unsigned fd, const char *buf, size_t count);

int sys_exit(int error_code);

long sys_getpid(void);
long sys_waitpid(int pid, int *status, int options);

struct pt_regs;
long sys_clone(struct pt_regs *regs);
long sys_execve(const char *filename, char *const argv[], char *const envp[]);

#endif
