#ifndef EXECVE_H
#define EXECVE_H

#include <elf.h>
#include <fs.h>
#include <printk.h>
#include <private_kdefs.h>
#include <mm.h>
#include <proc.h>
#include <malloc.h>

int do_execve(const char *filename, char *const argv[], char *const envp[]);

#endif // EXECVE_H