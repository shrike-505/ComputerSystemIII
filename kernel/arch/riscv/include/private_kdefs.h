#ifndef __PRIVATE_KDEFS_H__
#define __PRIVATE_KDEFS_H__

// QEMU virt 机器的时钟频率为 10 MHz
#define TIMECLOCK 10000000

#define PHY_START 0x80000000
#define PHY_SIZE 0x400000

#define OPENSBI_SIZE 0x200000

#define VM_START 0xffffffe000000000
#define VM_END 0xffffffff00000000
#define VM_SIZE (VM_END - VM_START)

#define PA2VA_OFFSET (VM_START - PHY_START)
#define PHY_END (PHY_START + PHY_SIZE)

#define PGSIZE 0x1000 // 4 KiB
#define PGROUNDDOWN(addr) ((addr) & ~(PGSIZE - 1))
#define PGROUNDUP(addr) PGROUNDDOWN((addr) + PGSIZE - 1)

#define USER_START 0x0        // user space start virtual address
#define USER_END 0x4000000000 // user space end virtual address

#endif
