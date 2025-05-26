    .section .text.init
    .globl _start
_start:
    
    # 0. 设置 sp 为 _sbss
    la sp, _sbss

    # 0.5. set vm
    call setup_vm
    call relocate
    
    call mm_init
    
    call setup_vm_final
    
    call task_init

    
    # 1. 将 stvec 设置为 _traps
    la a0, _traps
    csrw stvec, a0

    # 2. 设置 sie[STIE]
    li a0, 32
    csrs sie, a0

    # 3. 设置第一次时钟中断的时间
    call clock_set_next_event
    # rdtime a0
    # li t0, 10000000
    # add a0, a0, t0
    # li a6, 0
    # li a7, 0
    # ecall

    # 4. 设置 sstatus[SIE]
    li a0, 2
    csrs sstatus, a0

    # j _srodata

    # 6. 跳转到 start_kernel
    j start_kernel


relocate:
    # 1. set general purpose registers to appropriate values
    #    - set ra = ra + PA2VA_OFFSET
    #    - set sp = sp + PA2VA_OFFSET, if needed

    li t1, 0xffffffdf80000000
    add ra, ra, t1
    add sp, sp, t1

    # flush TLB
    sfence.vma zero, zero

    # 2. set satp to use early_pgtbl
    #    - set satp to use Sv39 mode

    la t0, early_pgtbl
    srli t0, t0, 12
    li t1, (8 << 60) # Sv39 mode
    or t0, t0, t1
    csrw satp, t0

    ret

    .section .bss.stack
    .space 0x1000