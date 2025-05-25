#include "private_kdefs.h"

    .section .bss.stack
    .space PGSIZE

    .section .text.init
    .globl _start
_start:
    # 0. 内存初始化
    la sp, _sbss
    call setup_vm
    call relocate
    call mm_init
    call setup_vm_final
    # 1. 将 stvec 设置为 _traps
    la t0, _traps
    csrw stvec, t0
    # 2. 设置 sie[STIE]
    li t0, 1 << 5
    csrs sie, t0
    # 3. 设置第一次时钟中断的时间
    rdtime t0
    li t1, TIMECLOCK
    add a0, t0, t1
    call _sbi_set_timer
    # # 4. 设置 sstatus[SIE]
    # li t0, 1 << 1
    # csrs sstatus, t0
    # 5. 跳转到 start_kernel
    call task_init
    call start_kernel

relocate:
    # 1. set general purpose registers to appropriate values
    #    - set ra = ra + PA2VA_OFFSET
    #    - set sp = sp + PA2VA_OFFSET, if needed

    li t0, PA2VA_OFFSET
    add ra, ra, t0
    add sp, sp, t0

    li t0, 0x1
    li t1, PA2VA_OFFSET
    la t2, _start
    sub t1, t1, t2
    add t1, t1, t0 
    csrw stvec, t1

    # flush TLB
    sfence.vma zero, zero

    # 2. set satp to use early_pgtbl
    #    - set satp to use Sv39 mode

    la t0, early_pgtbl
    li t1, 8
    srli t0, t0, 12
    slli t1, t1, 60
    or t0, t0, t1

    csrw satp, t0

    ret
 