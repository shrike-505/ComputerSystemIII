#ifndef __KSYSCALLS_H__
#define __KSYSCALLS_H__

#include <stddef.h>
#include <proc.h>

int sys_open(const char *pathname, int flags);
int sys_close(int fd);
long sys_read(unsigned fd, char *buf, size_t count);
long sys_write(unsigned fd, const char *buf, size_t count);
long sys_getpid(void);

struct pt_regs;
long sys_clone(struct pt_regs *regs);

#endif
