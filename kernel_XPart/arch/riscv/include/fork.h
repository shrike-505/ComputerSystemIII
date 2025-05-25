#ifndef FORK_H
#define FORK_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <proc.h>
#include <private_kdefs.h>
#include <syscalls.h>
#include <vm.h>
#include <mm.h>
#include <ksyscalls.h>
#include <printk.h>

long do_fork(struct pt_regs *regs);

#endif